/******************************************************************************\  
UTILITY:    PVcurve_post - PVCurve post processor  
STRUCTURE:  Standalone   
USAGE:      pvcurve <number> <bad-file#1> <bad-file#2> ... <bat-file#n>  
TYPE:       Post processes PVCurve output files  
SUMMARY:    Generate voltage reactance curves.  
RELATED:    PVCURVE  
UPDATED:    Version 1.00 6 October 2000  
            Version 1.01 22 October 2000  
            Version 1.02 4 November 2000  
LANGUAGE:   Standard C.  
DEVELOPER:  Walter Powell 360-418-8810   wlpowelljr@bpa.gov  
REQUESTER:  J Randall, G Comegys  
IPF:        Version 327 or above recommended.  
PURPOSE:    Post processes P-V curve data files, selecting the most critical  
            outage studies and rendering the output into an excel-importable  
            data file.   
\******************************************************************************/  
  
#include <stdio.h>  
#include <ctype.h>  
#include <string.h>  
#include <stdlib.h>  
#include <assert.h>  
#include <time.h>  
  
#define RECORDSIZE 132  
#define BUFFERSIZE 2048  
#define MAXCOLUMNS 200  
#define MAXROWS    500  
  
int temp_critical[MAXCOLUMNS], temp_pre_critical[MAXCOLUMNS],  
    temp_dvdq1[MAXROWS/3], temp_dvdq2[MAXROWS/3], temp_dvdq3[MAXROWS/3];  
char *progname, *VERSION = "1.02";  
char debug_filename[60], caselog_filename[60];  
FILE *debug_fp, *caselog_fp;  
  
typedef struct {  
  int solution;  
  char file[60];  
  char base[60];  
  char outage[60];  
  char source[60];  
} CRIT;  
  
typedef struct {  
  int count;  
  char *element[MAXCOLUMNS];  
} TOKN;  
  
CRIT critical[MAXCOLUMNS];  
CRIT pre_critical[MAXCOLUMNS];  
  
typedef struct {  
  char filename[12][60];  
  int  transpose_flag;  
} FILENM;  
  
typedef struct {  
  int row_count,  
      column_count,  
      num_null_columns,  
      num_null_rows,  
      column_status[MAXCOLUMNS],   
      row_status[MAXROWS];  
  char header[3][60],  
       *matrix[MAXROWS][MAXCOLUMNS];  
} MATRIX;  
  
typedef struct {  
  char *busname;  
  double max_min,  
         last_first,  
         second_last_first;  
} BS_DVDQ;  
  
BS_DVDQ bus_dvdq[MAXCOLUMNS/3];  
  
void usage_error_exit();  
FILE *efopen (char *, char *);  
int concat (int, int, MATRIX *);  
int load_pvdata (FILENM *, MATRIX *);  
int write_post_contingency (FILENM *, MATRIX *);  
int write_pre_contingency (FILENM *, MATRIX *);  
int parse_input_file (FILE *, char *, int *, int *);  
int compare_critical ( const void *, const void *);  
int compare_pre_critical ( const void *, const void *);  
int analyze_dvdq ( int, int, MATRIX * );  
int compare_dvdq1 ( const void *, const void *);  
int compare_dvdq2 ( const void *, const void *);  
int compare_dvdq3 ( const void *, const void *);  
int get_tokens (char *, TOKN *, char *, int);  
int read_next_record (char *, FILE *, int *, int *);  
char *extract_dir (char *);  
char *strtok_x (char *, char *);  
  
void usage_error_exit()  
{    
  fprintf (stderr, "Usage: post_pvcurve -c<num> [-d <debug_file>] [-t] <file#1n.bad> [<file#2.bad> ... <file#n.bad>]\n");  
  fprintf (stderr, "   -c<num> = number of critical cases selected (must be > 1) \n");  
  fprintf (stderr, "   -d <debug_file> = optional debug file\n");  
  fprintf (stderr, "   -t = generate transposed output file\n");  
  fprintf (stderr, "   <file#1.bad> ... <file#n.bad> - individual *.bad files\n");  
  exit (1);  
}  
  
/*----------------------------------------------------------------  
 *  
 * main:  Main program   
 *  
 *        1. Open and process input *.bad files  
 *        2. Select most critical cases  
 *        3. Processes each critical case, concatenating and optionally  
 *           transposing the data.  
 *  
 *----------------------------------------------------------------  
 * Routine structure:  
 *  
 * main                      -- main driver routine  
 *   usage_error_exit        -- usage error exit  
 *   parse_input_file        -- parse individual *.BAD files  
 *   qsort                   -- sort *.BAD structure  
 *   concat                  -- generate concatenated output files  
 *----------------------------------------------------------------  
 */  
  
int main(int argc, char *argv[])  
{  
  FILE *fptr[10];  
  char fname[10][60], *ptr,   
       *day[7] = {"Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat"},  
       *month[12] = {"Jan", "Feb", "Mar", "Apr", "May", "Jun",   
                     "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"};  
   
  int i, j, k, max_critical, transpose_flag = 0, num_input_files = 0,  
      status, num_critical = 0, num_pre_critical = 0, year, count;  
  time_t *tptr, vtptr;  
  struct tm *xtm;  
  
  MATRIX  pvdata[8];  
  
  progname = argv[0];  
  strcpy (debug_filename, "");  
  debug_fp = NULL;  
  
  fprintf (stderr, " Sizeof pvdata = %d\n", sizeof (pvdata));   
  
  for (i=0; i<8; i++) {  
    for (j=0; j<MAXROWS; j++) {  
      for (k=0; k<MAXCOLUMNS; k++) {  
        pvdata[i].matrix[j][k] = NULL;  
      }  
    }  
  }  
  
/*  
 * Process arguments  
 */  
  
  if (argc < 3) {  
    fprintf(stderr, "Missing arguments \n");  
    usage_error_exit();  
  }  
  for (i=0; i<10; i++) {  
    strcpy (fname[i], "");  
    fptr[i] = NULL;  
  }  
  for (i=1; i<argc; i++) {  
    if (strncmp (argv[i], "-c", 2) == 0) {  
      max_critical = atoi (&argv[i][2]);  
      if (max_critical < 1) {  
        fprintf(stderr, "Maximum critical studies must be > 0\n");  
        usage_error_exit();  
      }  
    } else if (strncmp (argv[i], "-d", 2) == 0) {  
      strcpy (debug_filename, argv[++i]);  
      debug_fp = efopen (debug_filename, "w");  
      if (debug_fp == NULL) {  
        fprintf(stderr, "Cannot open file %s\n", debug_filename);  
        usage_error_exit();  
      }  
    } else if (strncmp (argv[i], "-t", 2) == 0) {  
      transpose_flag = 1;  
    } else if (strncmp (argv[i], "-", 1) == 0) {  
      fprintf(stderr, "Unknown option [%s]\n", argv[i]);  
      usage_error_exit();  
    } else {  
      strcpy (fname[num_input_files], argv[i]);  
      fptr[num_input_files] = efopen (fname[num_input_files], "r");  
      if (fptr[num_input_files] == NULL) {  
        fprintf(stderr, "Cannot open file %s\n", fname[num_input_files]);  
        usage_error_exit();  
      } else {  
        status = parse_input_file (fptr[num_input_files], fname[num_input_files],  
                                   &num_critical, &num_pre_critical);  
      }  
      num_input_files++;  
    }  
  }  
  if (num_input_files == 0) {  
    fprintf(stderr, "No input files opened%s\n");  
    usage_error_exit();  
  } else {  
    for (i=0; i<num_input_files; i++) {  
      fclose (fptr[i]);  
    }  
  }  
  if (num_critical == 0) {  
    fprintf(stderr, "No critical data extracted from input files%s\n", fname[2]);  
    usage_error_exit();  
  } else if (num_critical >= MAXCOLUMNS) {  
    fprintf(stderr, "Too many critical data entities [%d] extracted from input files%s\n",  
      num_critical, fname[2]);  
    usage_error_exit();  
  } else if (max_critical > num_critical) {  
    fprintf(stderr, " [%d] 'case failed' entities found in *.BAD file(s)\n",  
      num_critical);  
    fprintf(stderr, " [%d] user-specified value is relaxed to [%d]\n",   
      max_critical, num_critical);  
    max_critical = num_critical;  
  }  
  
/* Sort struct critical by solution number */  
  
  for (i=0; i<MAXCOLUMNS; i++) temp_critical[i] = i;  
  qsort ((void *) temp_critical, (size_t) num_critical, sizeof (int),  
                  compare_critical);  
  
/* Sort struct pre_critical by solution number */  
  
  if (num_pre_critical > 0) {  
    for (i=0; i<MAXCOLUMNS; i++) temp_pre_critical[i] = i;  
    qsort ((void *) temp_pre_critical, (size_t) num_pre_critical, sizeof (int),  
                    compare_pre_critical);  
  }  
  
/* Generate caselog filename from first *.BAD case */  
  
  ptr = extract_dir (fname[0]);  
  strcpy (caselog_filename, ptr);  
  
  ptr = strstr (caselog_filename, ".BAD");  
  if (ptr == NULL) ptr = strstr (caselog_filename, ".bad");  
  if (ptr != NULL) {  
    *ptr = '\x00';  
    strcat (caselog_filename, "XX.SUM");  
  } else {  
    strcpy (caselog_filename, "NONAMEXX.SUM");  
  }  
  caselog_fp = efopen (caselog_filename, "w");  
  
  tptr = &vtptr;  
  time(tptr);  
  xtm = localtime(tptr);  
  
  fprintf (caselog_fp, " Case summary of Post-PVCURVE version %s\n", VERSION);  
  year = 1900 + xtm->tm_year;  
  fprintf (caselog_fp, " Executed on %s %2d-%s-%d at %2d:%2d:%2d\n\n",  
    day[xtm->tm_wday], xtm->tm_mday, month[xtm->tm_mon], year, xtm->tm_hour,   
    xtm->tm_min, xtm->tm_sec);  
  fprintf (caselog_fp, " Sorted contingency cases from input *.BAD files\n\n");  
  fprintf (caselog_fp, " Solution   Outage                      File\n\n");  
  
  for (i=0; i<num_critical; i++) {  
    j = temp_critical[i];  
    fprintf (caselog_fp, "     %3d    %-26s %s\n", critical[j].solution,   
      critical[j].outage, critical[j].file);  
  }  
  
  fprintf (caselog_fp, "\n Sorted pre outage cases from input *.BAD files\n\n");  
  fprintf (caselog_fp, " Solution   Outage                      File\n\n");  
  
  for (i=0; i<num_pre_critical; i++) {  
    j = temp_pre_critical[i];  
    fprintf (caselog_fp, "     %3d    %-26s %s\n", pre_critical[j].solution,   
      pre_critical[j].outage, pre_critical[j].file);  
  }  
  
  for (i=0; i<max_critical; i++) {  
    j = temp_critical[i];  
    status = concat (j, transpose_flag, pvdata);  
  }  
  
  
  fclose (caselog_fp);  
  fprintf (stderr, " Case summary saved on file %s\n", caselog_filename);  
  
  return 1;  
}  
  
FILE *efopen(char *file, char *mode) /* fopen file, die if can't */  
{  
  FILE *fp;  
  if ((fp = fopen(file, mode)) == NULL) {  
    fprintf(stderr, "%s: Can't open file %s mode %s\n",  progname, file, mode);  
    exit (1);  
  }  
  return fp;  
}  
  
/*----------------------------------------------------------------  
 *  
 * concat:  Concatenate PVCURVE files  
 *  
 *        1. Open 4 pre-contingency cases and 4 post-contingency cases  
 *        2. Merge two sets of files according  
 *           file 1: (PRE MW) + (POST DV) (PRE KV) + (POST KV) + (PRE QQ) + (POST QQ)  
 *                2: (PRE MW) + (PRE DV) + (PRE QQ) + (PRE KV)  
 *        3. Write new files above as type *CNNXX.TXT and *_0XX.TXT (normal)  
 *           and *CNNXXT.TXT and *_0XXT.TXT (transposed).  
 *  
 *----------------------------------------------------------------  
 * Routine structure:  
 *  
 * concat                    -- main driver routine  
 *   usage_error_exit        -- usage error exit  
 *   parse_input_file        -- parse individual *.BAD files  
 *   load_pvdata             -- Load all data into a matrix structure  
 *   write_concat            -- Write concatenated data files  
 *----------------------------------------------------------------  
 */  
  
int concat (int critical_index, int transpose_flag, MATRIX *pvdata)  
{  
  char tempname[60], newtempname[60], *ptr, *name;   
  int i, len, status, num_row, num_column;  
  static int num_new_pre_cntg_files = 0;  
  FILENM filenm;  
   
  
  name = critical[critical_index].file;  
  filenm.transpose_flag = transpose_flag;  
  strcpy (tempname, name);  
  ptr = strstr (tempname, ".TXT");  
  if (ptr == NULL) ptr = strstr (tempname, ".txt");  
  if (ptr == NULL) {  
    fprintf (stderr, "Cannot parse .TXT from input file %s\n", name);  
    usage_error_exit();  
  } else {  
    ptr--;  
    ptr--;  
    *ptr = '\x00';  
  }  
  
  ptr = extract_dir (tempname);   /* ptr contains file name sans directory */  
  strcpy (newtempname, ptr);  
  
/* Generate complete set of post-contintency file names */  
  
  strcpy (filenm.filename[4], tempname);  
  strcpy (filenm.filename[5], tempname);  
  strcpy (filenm.filename[6], tempname);  
  strcpy (filenm.filename[7], tempname);  
  strcat (filenm.filename[4], "MW.TXT");  
  strcat (filenm.filename[5], "DV.TXT");  
  strcat (filenm.filename[6], "KV.TXT");  
  strcat (filenm.filename[7], "QQ.TXT");  
  strcpy (filenm.filename[8], newtempname);  
  strcpy (filenm.filename[10], newtempname);  
  strcat (filenm.filename[8], "XX.TXT");  
  strcat (filenm.filename[10], "XXT.TXT");  
  
/* Generate complete set of pre-contintency file names */  
  
/* Eliminate character C and digits NN in filename ...CNNMW.TXT */  
  
  len = strlen (tempname);  
  while (len > 0 && isdigit (tempname[len-1])) len--;  
  while (len > 0 && tempname[len-1] == 'C') len--;  
  tempname[len++] = '_';  
  tempname[len++] = '0';  
  tempname[len] = '\x00';  
  
  ptr = extract_dir (tempname);   /* ptr contains file name sans directory */  
  strcpy (newtempname, ptr);  
  
  strcpy (filenm.filename[0], tempname);  
  strcpy (filenm.filename[1], tempname);  
  strcpy (filenm.filename[2], tempname);  
  strcpy (filenm.filename[3], tempname);  
  strcat (filenm.filename[0], "MW.TXT");  
  strcat (filenm.filename[1], "DV.TXT");  
  strcat (filenm.filename[2], "KV.TXT");  
  strcat (filenm.filename[3], "QQ.TXT");  
  strcpy (filenm.filename[9], newtempname);  
  strcpy (filenm.filename[11], newtempname);  
  strcat (filenm.filename[9], "XX.TXT");  
  strcat (filenm.filename[11], "XXT.TXT");  
/*  
 * Load and filter input data   
 */  
  
  status = load_pvdata (&filenm, pvdata);  
  
  if (num_new_pre_cntg_files == 0) {  
    status = analyze_dvdq (critical_index, 1, pvdata);  /* Summarize pre-contingency dV/dQ */  
  }  
  
/*   
 * Write file summary  
 */  
  
  num_row = pvdata[4].row_count;  
  num_column = pvdata[0].column_count + 3 * pvdata[1].column_count  
                                      + 2 * pvdata[3].column_count - 10  
                                      - pvdata[3].num_null_columns  
                                      - pvdata[7].num_null_columns;  
  
  fprintf (caselog_fp, "------------------------------------------------------\n");  
  fprintf (caselog_fp, " New post-contingency master file %s %d rows %d columns\n",   
    filenm.filename[8], num_row, num_column);  
  
  if (filenm.transpose_flag == 1) {  
  
    fprintf (caselog_fp, " New transposed post-contingency master file %s %d rows %d columns\n",   
      filenm.filename[10], num_row, num_column);  
  
  }  
  
  for (i=0; i<1; i++)  fprintf (caselog_fp, " Input Pre-contingency files   %s\n",   
    filenm.filename[i]);    
  for (i=1; i<4; i++)  fprintf (caselog_fp, "                               %s\n",   
    filenm.filename[i]);    
  for (i=4; i<5; i++)  fprintf (caselog_fp, " Input Post-contingency files  %s\n",   
    filenm.filename[i]);    
  for (i=5; i<8; i++)  fprintf (caselog_fp, "                               %s\n",   
    filenm.filename[i]);    
  
  fprintf (caselog_fp, "------------------------------------------------------\n");  
  
/*  
 * Write a post_contingency file *XX.TXT with structure  
 *  
 *  (PRE MW) + (POST DV) + (PRE KV) + (POST KV) + (PRE QQ) + (POST QQ)  
 *     [0]        [5]        [2]         [6]       [3]         [7]  
 *  
 */  
  
 status = write_post_contingency ( &filenm, pvdata);  
 status = analyze_dvdq (critical_index, 5, pvdata);  /* Summarize post-contingency dV/dQ */  
  
/*  
 * Once only write a pre_contingency file *_0YY.TXT with structure  
 *  
 *  (PRE MW) + (PRE DV) + (PRE KV) + (POST QQ)  
 *    [0]         [1]        [2]       [3]  
 */  
  
  if (num_new_pre_cntg_files++ == 0) {  
    status = write_pre_contingency ( &filenm, pvdata);  
  }  
  
  return 0;  
}  
  
/*----------------------------------------------------------------  
 *  
 * load_pvdata:  Process and filter all data   
 *  
 *        1. Open 4 pre-contingency cases and 4 post-contingency cases  
 *           File index  Description   Quantity  
 *           ----------  ------------  ---------  
 *              0        PRE MW        Flowgate MW  
 *              1        PRE DVDQ      Bus dv/dq  
 *              2        PRE KV        Bus kV  
 *              3        PRE QQ        Generator MVAR  
 *              4        POST MW       Flowgate MW  
 *              5        POST DVDQ     Bus dv/dq  
 *              6        POST KV       Bus kV  
 *              7        POST QQ       Generator MVAR  
 *  
 *----------------------------------------------------------------  
 * Routine structure:  
 *  
 * load_pvdata                    -- main driver routine  
 *----------------------------------------------------------------  
 */  
  
int load_pvdata (FILENM *filenm, MATRIX pvdata[])  
{  
  char inbuf[8][BUFFERSIZE], *ptr;   
  int len, i, j, k, num_row, status, max_token, count, flag;  
  FILE *f[12];  
  TOKN token;  
  static int entry = 0;  
  entry++;  
  
/* Free dynamic memory from previous outage */  
  
  if (entry > 1) {  
    count = 0;  
    for (i=0; i<8; i++) {  
      for (j=0; j<MAXROWS; j++) {  
        for (k=0; k<MAXCOLUMNS; k++) {  
          ptr = pvdata[i].matrix[j][k];  
          if (ptr != NULL) {  
            count += strlen (ptr);  
            free (ptr);  
          }  
        }  
      }  
    }  
    fprintf (stderr, " %d bytes of pvdata freed \n", count);  
  }  
  
/* Open all input files */  
  
  for (i=0; i<8; i++) {  
    f[i] = efopen(filenm->filename[i], "r");  
  }  
  
/* Pass 1 - process pre-contingency files */  
  
  num_row = 0;  
  while (fgets(inbuf[0], sizeof inbuf[0], f[0]) != NULL &&  
         fgets(inbuf[1], sizeof inbuf[1], f[1]) != NULL &&  
         fgets(inbuf[2], sizeof inbuf[2], f[2]) != NULL &&  
         fgets(inbuf[3], sizeof inbuf[3], f[3]) != NULL) {  
  
    num_row++;  
    if (num_row < 4) {  
      for (i=0; i<4; i++) {  
        len = strlen (inbuf[i]);  
        if (len > 60) inbuf[i][60] = '\x00';  
        strcpy (pvdata[i].header[num_row], inbuf[i]);  
      }  
    } else {  
      max_token = 0;  
      for (i=0; i<4; i++) {  
        status = get_tokens (inbuf[i], &token, filenm->filename[i], num_row);  
        for (j=0; j<token.count; j++) {  
          len = strlen (token.element[j]);  
          ptr = malloc (len+1);  
          if (ptr == NULL) {  
            fprintf (stderr, "Cannot allocate memory - file %s element[%d] %s \n",  
              filenm->filename[i], j, token.element[j]);  
            exit (1);  
          } else {  
            pvdata[i].matrix[num_row-4][j] = ptr;  
            strcpy (ptr, token.element[j]);  
          }  
        }  
        pvdata[i].column_count = token.count;  
        if (token.count > max_token) max_token = token.count;  
      }  
    }  
  }    
  pvdata[0].row_count = num_row - 3;  
  
/* Pass 2 - process post-contingency files */  
  
  num_row = 0;  
  while (fgets(inbuf[4], sizeof inbuf[4], f[4]) != NULL &&  
         fgets(inbuf[5], sizeof inbuf[5], f[5]) != NULL &&  
         fgets(inbuf[6], sizeof inbuf[6], f[6]) != NULL &&  
         fgets(inbuf[7], sizeof inbuf[7], f[7]) != NULL) {  
  
    num_row++;  
    if (num_row < 4) {  
      for (i=4; i<8; i++) {  
        len = strlen (inbuf[i]);  
        if (len > 60) inbuf[i][60] = '\x00';  
        strcpy (pvdata[i].header[num_row], inbuf[i]);  
      }  
    } else {  
      max_token = 0;  
      for (i=4; i<8; i++) {  
        status = get_tokens (inbuf[i], &token, filenm->filename[i], num_row);  
        for (j=0; j<token.count; j++) {  
          len = strlen (token.element[j]);  
          ptr = malloc (len+1);  
          if (ptr == NULL) {  
            fprintf (stderr, "Cannot allocate memory - file %s element[%d] %s \n",  
              filenm->filename[i], j, token.element[j]);  
            exit (1);  
          } else {  
            pvdata[i].matrix[num_row-4][j] = ptr;  
            strcpy (ptr, token.element[j]);  
          }  
        }  
        pvdata[i].column_count = token.count;  
        if (token.count > max_token) max_token = token.count;  
      }  
    }  
  }    
  pvdata[4].row_count = num_row - 3;  
  
/* Close all input files */  
  
  for (i=0; i<8; i++) {  
    fclose (f[i]);  
  }  
  
/* Flag null Qgen/Qres columns */  
  
  for (i=2; i<pvdata[3].column_count; i++) {  
    count = 0;  
    pvdata[3].num_null_rows = 0;  
    pvdata[3].num_null_columns = 0;  
    pvdata[7].num_null_rows = 0;  
    pvdata[7].num_null_columns = 0;  
    for (j=3; j<pvdata[4].row_count; j++) {  
      if (atof (pvdata[3].matrix[j][i]) != 0.0 ||  
          atof (pvdata[7].matrix[j][i]) != 0.0) count++;  
    }  
    if (count > 0) {  
      pvdata[3].column_status[i] = 1;  
      pvdata[7].column_status[i] = 1;  
    } else {  
      pvdata[3].column_status[i] = 0;  
      pvdata[3].num_null_columns++;  
      pvdata[7].column_status[i] = 0;  
      pvdata[7].num_null_columns++;  
/*    fprintf (stderr, "Null QQ column %d flagged\n", i);  */  
    }  
  }  
  
/* Ignore columns which have all negative dV/dQ rows entities */  
  
  for (j=2; j<pvdata[1].column_count; j++) {  
    count = 0;  
    flag = 0;  
    for (i=3; i<pvdata[0].row_count; i++) {  
      if (atof (pvdata[1].matrix[i][j]) < 0.0) {  
        count++;  
        if (flag == 0) flag = i;  
      }  
    }  
    if (count < pvdata[0].row_count - 3 || pvdata[0].row_count == 0) {  
      pvdata[1].column_status[j] = 1;  
    } else {  
      pvdata[1].column_status[j] = 0;  
      fprintf (stderr, "Entire column %s consists of negative dV/dQ entities, file %s. Column is ignored\n",   
        pvdata[1].matrix[2][j], filenm->filename[1]);   
    }  
  }  
  for (j=2; j<pvdata[5].column_count; j++) {  
    count = 0;  
    flag = 0;  
    for (i=3; i<pvdata[4].row_count; i++) {  
      if (atof (pvdata[5].matrix[i][j]) < 0.0) {  
        count++;  
        flag = i;  
      }  
    }  
    if (count < pvdata[5].row_count - 3 || pvdata[5].row_count == 0) {  
      pvdata[5].column_status[j] = 1;  
    } else {  
      pvdata[5].column_status[j] = 0;  
      fprintf (stderr, "Entire column %s consists of negative dV/dQ entities, file %s. Column is ignored\n",   
        pvdata[5].matrix[2][j], filenm->filename[5]);   
    }  
  }  

/* Flag negative dV/dQ rows columns */  
  
  for (i=3; i<pvdata[0].row_count; i++) {  
    count = 0;  
    flag = 0;  
    for (j=2; j<pvdata[1].column_count; j++) {  
      if (atof (pvdata[1].matrix[i][j]) < 0.0 && pvdata[1].column_status[j] == 1) {  
        count++;  
        if (flag == 0) flag = j;  
      }  
    }  
    if (count == 0) {  
      pvdata[1].row_status[i] = 1;  
    } else {  
      pvdata[1].row_status[i] = 0;  
      fprintf (stderr, "Negative dV/dQ encountered in row %s, column %s, file %s. Row is ignored\n",   
        pvdata[1].matrix[i][0], pvdata[1].matrix[2][flag], filenm->filename[1]);   
      strncpy (pvdata[0].matrix[i][0], "IGNORE ***", 10);  
    }  
  }  
  for (i=3; i<pvdata[4].row_count; i++) {  
    count = 0;  
    flag = 0;  
    for (j=2; j<pvdata[5].column_count; j++) {  
      if (atof (pvdata[5].matrix[i][j]) < 0.0 && pvdata[5].column_status[j] == 1) {  
        count++;  
        flag = j;  
      }  
    }  
    if (count == 0) {  
      pvdata[5].row_status[i] = 1;  
    } else {  
      pvdata[5].row_status[i] = 0;  
      fprintf (stderr, "Negative dV/dQ encountered in row %s, column %s, file %s. Row is ignored\n",   
        pvdata[5].matrix[i][0], pvdata[5].matrix[2][flag], filenm->filename[5]);   
      strncpy (pvdata[4].matrix[i][0], "IGNORE ***", 10);  
    }  
  }  
  
  return 0;  
}  
  
/*----------------------------------------------------------------  
 *  
 * write_post_contingency:    
 *  
 *   Write concatenated Post-contingency case and optional  
 *   transposed Post-contingency case.  The normal file is  
 *   represented in the following order.  
 *  
 *   (PRE MW) + (POST DV) + (PRE KV) + (POST KV) + (PRE QQ) + (POST KV)  
 *  
 *----------------------------------------------------------------  
 * Routine structure:  
 *  
 * concat                    -- main driver routine  
 *   usage_error_exit        -- usage error exit  
 *----------------------------------------------------------------  
 */  
  
int write_post_contingency (FILENM *filenm, MATRIX pvdata[])  
{  
  char outbuf[6*BUFFERSIZE];   
  int i, j;  
  FILE *f[12];  
  
/*  
 * Write concatenated post-contingency file  
 */  
  
  f[8] = efopen(filenm->filename[8], "w");  
  
  for (i=0; i<3; i++) fprintf (f[8], "%s", pvdata[4].header[i]);  
  strcpy (outbuf, "");  
  
/*  
 * Write (PRE MW) data segment by rows  
 */  
  
  for (i=0; i<pvdata[4].row_count; i++) {  
    strcpy (outbuf, "");  
    strcat (outbuf, pvdata[4].matrix[i][0]);   /* STEP    */  
    strcat (outbuf, " | ");  
  
    for (j=1; j<pvdata[0].column_count; j++) {  
      strcat (outbuf, pvdata[0].matrix[i][j]);   /* PRE-MW */  
      strcat (outbuf, " | ");  
    }  
/*  
 * Write (POST DV) + (PRE KV) + (POST KV) data segment by rows  
 */  
    for (j=2; j<pvdata[5].column_count; j++) {  
      strcat (outbuf, pvdata[5].matrix[i][j]);   /* POST DV */  
      strcat (outbuf, " | ");  
      strcat (outbuf, pvdata[2].matrix[i][j]);   /* PRE KV  */  
      strcat (outbuf, " | ");  
      strcat (outbuf, pvdata[6].matrix[i][j]);   /* POST KV */  
      strcat (outbuf, " | ");  
    }  
/*  
 * Write (PRE QQ) + (POST QQ) data segment by rows  
 */  
    for (j=2; j<pvdata[3].column_count; j++) {  
      if (pvdata[3].column_status[j] > 0) {  
        strcat (outbuf, pvdata[3].matrix[i][j]);   /* PRE QQ  */  
        strcat (outbuf, " | ");  
        strcat (outbuf, pvdata[7].matrix[i][j]);   /* POST QQ  */  
        strcat (outbuf, " | ");  
      }  
    }  
    fprintf (f[8], "%s\n", outbuf);  
  }  
  fclose (f[8]);  
  
  if (filenm->transpose_flag == 1) {  
  
/*   
 * Write transposed post-contingency file  
 */  
  
    f[10] = efopen(filenm->filename[10], "w");  
  
    for (i=0; i<3; i++) fprintf (f[10], "%s", pvdata[0].header[i]);  
    strcpy (outbuf, "");  
  
/*  
 *  Write (PRE MW) data segment by columns  
 */  
  
    for (i=0; i<pvdata[0].column_count; i++) {  
      strcpy (outbuf, "");  
      strcat (outbuf, pvdata[4].matrix[0][i]);   /* STEP  */  
      strcat (outbuf, " | ");  
      for (j=1; j<pvdata[4].row_count; j++) {  
        strcat (outbuf, pvdata[0].matrix[j][i]);   /* PRE-MW */  
        strcat (outbuf, " | ");  
      }  
      fprintf (f[10], "%s\n", outbuf);  
    }  
/*  
 *  Write (POST DV) + (PRE KV) + (POST KV) data segment by columns  
 */  
    for (i=2; i<pvdata[1].column_count; i++) {  
      strcpy (outbuf, "");  
      for (j=0; j<pvdata[4].row_count; j++) {  
        strcat (outbuf, pvdata[5].matrix[j][i]);   /* POST DVDQ */  
        strcat (outbuf, " | ");  
      }  
      fprintf (f[10], "%s\n", outbuf);  
  
      strcpy (outbuf, "");  
      for (j=0; j<pvdata[4].row_count; j++) {  
        strcat (outbuf, pvdata[2].matrix[j][i]);   /* PRE KV    */  
        strcat (outbuf, " | ");  
      }  
      fprintf (f[10], "%s\n", outbuf);  
  
      strcpy (outbuf, "");  
      for (j=0; j<pvdata[4].row_count; j++) {  
        strcat (outbuf, pvdata[6].matrix[j][i]);   /* POST KV   */  
        strcat (outbuf, " | ");  
      }  
      fprintf (f[10], "%s\n", outbuf);  
  
    }  
/*  
 *  Write (PRE QQ) + (POST QQ) data segment by columns  
 */  
    for (i=2; i<pvdata[3].column_count; i++) {  
  
      if (pvdata[3].column_status[i] > 0) {  
        strcpy (outbuf, "");  
        for (j=0; j<pvdata[4].row_count; j++) {  
          strcat (outbuf, pvdata[3].matrix[j][i]);   /* PRE QQ    */  
          strcat (outbuf, " | ");  
        }  
        fprintf (f[10], "%s\n", outbuf);  
      }  
  
      if (pvdata[3].column_status[i] > 0) {  
        strcpy (outbuf, "");  
        for (j=0; j<pvdata[4].row_count; j++) {  
          strcat (outbuf, pvdata[7].matrix[j][i]);   /* POST QQ   */  
          strcat (outbuf, " | ");  
        }  
        fprintf (f[10], "%s\n", outbuf);  
      }  
    }  
    fclose (f[8]);  
  }  
  
  return 0;  
}  
  
/*----------------------------------------------------------------  
 *  
 * write_pre_contingency:    
 *  
 *   Write concatenated Pre-contingency case and optional  
 *   transposed Post-contingency case.  The normal file is  
 *   represented in the following order.  
 *  
 *   (PRE MW) + (PRE DV) + (PRE KV) + (PRE QQ)  
 *  
 *----------------------------------------------------------------  
 * Routine structure:  
 *  
 * concat                    -- main driver routine  
 *   usage_error_exit        -- usage error exit  
 *----------------------------------------------------------------  
 */  
  
int write_pre_contingency (FILENM *filenm, MATRIX pvdata[])  
{  
  char outbuf[6*BUFFERSIZE];   
  int i, j;  
  FILE *f[12];  
  
/*  
 * Write concatenated pre-contingency file  
 */  
  
  f[9] = efopen(filenm->filename[9], "w");  
  
  for (i=0; i<3; i++) fprintf (f[9], "%s", pvdata[0].header[i]);  
  strcpy (outbuf, "");  
  
/*  
 * Write (PRE MW) data segment by rows  
 */  
  
  for (i=0; i<pvdata[0].row_count; i++) {  
    strcpy (outbuf, "");  
    for (j=0; j<pvdata[0].column_count; j++) {  
      strcat (outbuf, pvdata[0].matrix[i][j]);   /* PRE-MW */  
      strcat (outbuf, " | ");  
    }  
/*  
 * Write (PRE DV) + (PRE KV) data segment br rows  
 */  
    for (j=2; j<pvdata[1].column_count; j++) {  
      strcat (outbuf, pvdata[1].matrix[i][j]);   /* PRE DV */  
      strcat (outbuf, " | ");  
      strcat (outbuf, pvdata[2].matrix[i][j]);   /* PRE KV  */  
      strcat (outbuf, " | ");  
    }  
/*  
 * Write (PRE QQ) data segment by rows  
 */  
    for (j=2; j<pvdata[3].column_count; j++) {  
      if (pvdata[3].column_status[j] > 0) {  
        strcat (outbuf, pvdata[3].matrix[i][j]);   /* PRE QQ  */  
        strcat (outbuf, " | ");  
      }  
    }  
    fprintf (f[9], "%s\n", outbuf);  
  }  
  fclose (f[9]);  
  
  if (filenm->transpose_flag == 1) {  
  
/*   
 * Write transposed pre-contingency file  
 */  
  
    f[11] = efopen(filenm->filename[11], "w");  
  
    for (i=0; i<3; i++) fprintf (f[10], "%s", pvdata[0].header[i]);  
    strcpy (outbuf, "");  
  
/*  
 *  Write (PRE MW) data segment by columns  
 */  
  
    for (i=0; i<pvdata[0].column_count; i++) {  
      strcpy (outbuf, "");  
      for (j=0; j<pvdata[0].row_count; j++) {  
        strcat (outbuf, pvdata[0].matrix[j][i]);   /* PRE-MW */  
        strcat (outbuf, " | ");  
      }  
      fprintf (f[11], "%s\n", outbuf);  
    }  
/*  
 *  Write (PRE DV) + (PRE KV) data segment by columns  
 */  
    for (i=2; i<pvdata[1].column_count; i++) {  
  
      strcpy (outbuf, "");  
      for (j=0; j<pvdata[0].row_count; j++) {  
        strcat (outbuf, pvdata[1].matrix[j][i]);   /* PRE DVDQ */  
        strcat (outbuf, " | ");  
      }  
      fprintf (f[11], "%s\n", outbuf);  
  
      strcpy (outbuf, "");  
      for (j=0; j<pvdata[0].row_count; j++) {  
        strcat (outbuf, pvdata[3].matrix[j][i]);   /* PRE KV    */  
        strcat (outbuf, " | ");  
      }  
      fprintf (f[11], "%s\n", outbuf);  
  
    }  
/*  
 *  Write (PRE QQ) data segment by columns  
 */  
    for (i=2; i<pvdata[3].column_count; i++) {  
  
      if (pvdata[3].column_status[i] > 0) {  
        strcpy (outbuf, "");  
        for (j=0; j<pvdata[0].row_count; j++) {  
          strcat (outbuf, pvdata[3].matrix[j][i]);   /* PRE QQ    */  
          strcat (outbuf, " | ");  
        }  
        fprintf (f[11], "%s\n", outbuf);  
      }  
  
    }  
    fclose (f[11]);  
  }  
  
  return 0;  
}  
  
/*----------------------------------------------------------------  
 *  
 * Integer function parse_input_files loads the contents of *.BAD  
 * files into structure critical for subsequent processing  
 *  
 *----------------------------------------------------------------  
 * Calling parameters:  
 *  
 * Parameters:  
 *     *f1  - file pointer to input file  
 *     *num - current index to stucture critical  
 *----------------------------------------------------------------  
 */  
  
int parse_input_file (FILE *f1, char *fname, int *num_critical,   
                      int *num_pre_critical)  
{  
  int i, len, status = 0, eof, first, ntok, num = 0;  
  char inbuf[RECORDSIZE], *token[50], *ptr, directory[60];  
  
/* Extract the VAX or Linux directory name from the file name */  
  
  strcpy (directory, fname);  
  len = strlen (directory);  
  i = len-1;  
  while (i > 0 && directory[i] != ']' && directory[i] != '/' && directory[i] != '\\') i--;  
  if (directory[i] == ']' || directory[i] == '/' || directory[i] == '\\') {  
    directory[++i] = '\x00';  
  } else {  
    directory[i] = '\x00';  
  }  
  strcpy (critical[*num_critical].source, fname);  
  
  status = read_next_record (inbuf, f1, &eof, &num);  
  if (eof == 0) {  
    fprintf (stderr, "Error reading input file %s\n", fname);  
    usage_error_exit();  
  }  
  fprintf (stderr, " Loading file %s - %s\n", fname, inbuf);  
  first = 0;  
  eof = 1;  
  while (eof == 1) {  
    status = read_next_record (inbuf, f1, &eof, &num);  
    if (eof == 1 && strncmp (inbuf, "Case failed at",   
                             strlen("Case failed at")) == 0) {  
      ntok = 0;  
      ptr = strstr (inbuf, "\r");  
      while (ptr != NULL) {  
        *ptr = ' ';  
        ptr = strstr (inbuf, "\r");  
      }     
      ptr = strstr (inbuf, "\n");  
      while (ptr != NULL) {  
        *ptr = ' ';  
        ptr = strstr (inbuf, "\n");  
      }     
      ptr = strtok (inbuf, " ");  
      while (ptr != NULL) {  
        token[ntok++] = ptr;  
        ptr = strtok (NULL, " ");  
      }  
      assert (ntok == 12);  
      critical[*num_critical].solution = atof (token[4]);  
      strcpy (critical[*num_critical].file, directory);  
      strcat (critical[*num_critical].file, token[ntok-1]);  
      status = read_next_record (inbuf, f1, &eof, &num);  
  
      if (eof == 0) {  
        fprintf (stderr, "Missing trailer record %din input file %s\n", fname);  
        usage_error_exit();  
      } else {  
        ptr = strtok (inbuf, " ");  
        while (ptr != NULL) {  
          token[ntok++] = ptr;  
          ptr = strtok (NULL, " ");  
        }  
        strcpy (critical[*num_critical].base, token[12]);  
        strcpy (critical[*num_critical].outage, token[14]);  
        (*num_critical)++;  
      }  
    } else if (eof == 1 && strncmp (inbuf, "Pre Outage Case failed at",   
                                    strlen("Pre Outage Case failed at")) == 0) {  
      ntok = 0;  
      ptr = strstr (inbuf, "\r");  
      while (ptr != NULL) {  
        *ptr = ' ';  
        ptr = strstr (inbuf, "\r");  
      }     
      ptr = strstr (inbuf, "\n");  
      while (ptr != NULL) {  
        *ptr = ' ';  
        ptr = strstr (inbuf, "\n");  
      }     
      ptr = strtok (inbuf, " ");  
      while (ptr != NULL) {  
        token[ntok++] = ptr;  
        ptr = strtok (NULL, " ");  
      }  
      assert (ntok == 10);  
      pre_critical[*num_pre_critical].solution = atof (token[6]);  
      strcpy (pre_critical[*num_pre_critical].file, directory);  
      strcat (pre_critical[*num_pre_critical].file, token[ntok-1]);  
      status = read_next_record (inbuf, f1, &eof, &num);  
  
      if (eof == 0) {  
        fprintf (stderr, "Missing trailer record %din input file %s\n", fname);  
        usage_error_exit();  
      } else {  
        ptr = strtok (inbuf, " ");  
        while (ptr != NULL) {  
          token[ntok++] = ptr;  
          ptr = strtok (NULL, " ");  
        }  
        strcpy (pre_critical[*num_pre_critical].base, token[10]);  
        strcpy (pre_critical[*num_pre_critical].outage, "{Pre-Contingency}");  
        (*num_pre_critical)++;  
      }  
    }  
  }  
  return 0;  
}  
  
/*----------------------------------------------------------------  
 *  
 * Integer function get_tokens loads parses the input character  
 * string into tokens demarcated with "|".  
 *  
 *----------------------------------------------------------------  
 * Calling parameters:  
 *  
 * Parameters:  
 *     *buffer - character string to be parsed  
 *     *token  - parsed tokens  
 *     *filename - parsed file name for debug purposes  
 *----------------------------------------------------------------  
 */  
  
int get_tokens (char *buffer, TOKN *token, char *filename, int num_row)  
{  
  int ntok = 0;  
  char *ptr;  
    
/* replace any DOS '\r' with ' ' */  
  
  ptr = strstr (buffer, "\r");  
  while (ptr != NULL) {  
    *ptr = ' ';  
    ptr = strstr (buffer, "\r");  
  }  
  ptr = strstr (buffer, "\n");  
  while (ptr != NULL) {  
    *ptr = ' ';  
    ptr = strstr (buffer, "\n");  
  }  
  ptr = strtok_x (buffer, "|");  
  while (ptr != NULL) {  
    token->element[ntok] = ptr;  
  
    if (debug_fp != NULL) {  
      fprintf (debug_fp, " File %s record %d token[%d] = [%s]\n",   
        filename, num_row, ntok, token->element[ntok]);  
    }  
  
    ntok++;  
    token->element[ntok] = NULL;  
    ptr = strtok_x (NULL, "|");  
  }  
  token->count = ntok;  
  return 0;  
}  
  
  
/*----------------------------------------------------------------  
 *  
 * Integer function read_next_record reads single records from the input  
 * files.  
 *  
 *----------------------------------------------------------------  
 * Calling parameters:  
 *  
 * Parameters:  
 *     *f1  - file pointer to input file  
 *     *num - current index to stucture critical  
 *----------------------------------------------------------------  
 */  
  
int read_next_record (char *inbuf, FILE *f1, int *loop, int *numrec)  
{  
  char *ptr;  
  
  ptr = fgets (inbuf, RECORDSIZE, f1);  
  
  if (debug_fp != NULL) {  
    fprintf (debug_fp, "Record  %5d [%s]\n", ++(*numrec), inbuf);  
  }  
  
  *loop = (ptr == NULL) ? 0 : 1;  
  return 0;  
}  
  
  
/*----------------------------------------------------------------  
 *  
 * Integer function compare_critical performs the qsort comparison for  
 * sorting structure critical_files by the field "solution".  
 *  
 *----------------------------------------------------------------  
 * Calling parameters:  
 *  
 * Parameters:  
 *     op1  - pointer to critical_files[] from pointer in temp_critical[]  
 *     op2  - pointer to critical_files[] from pointer in temp_critical[]  
 *----------------------------------------------------------------  
 */  
  
int compare_critical ( const void *op1, const void *op2 )  
{  
  const int *p1 = (const int *) op1;  
  const int *p2 = (const int *) op2;  
  int compare, solution1, solution2;  
  solution1 = critical[*p1].solution;  
  solution2 = critical[*p2].solution;  
  compare = solution1 - solution2;  
  return compare;  
}  
  
/*----------------------------------------------------------------  
 *  
 * Integer function compare_pre_critical performs the qsort comparison for  
 * sorting structure pre_critical_files by the field "solution".  
 *  
 *----------------------------------------------------------------  
 * Calling parameters:  
 *  
 * Parameters:  
 *     op1  - pointer to critical_files[] from pointer in temp_pre_critical[]  
 *     op2  - pointer to critical_files[] from pointer in temp_pre_critical[]  
 *----------------------------------------------------------------  
 */  
  
int compare_pre_critical ( const void *op1, const void *op2 )  
{  
  const int *p1 = (const int *) op1;  
  const int *p2 = (const int *) op2;  
  int compare, solution1, solution2;  
  solution1 = pre_critical[*p1].solution;  
  solution2 = pre_critical[*p2].solution;  
  compare = solution1 - solution2;  
  return compare;  
}  
  
/*----------------------------------------------------------------  
 *  
 * Character function strtok_x is a variation of the standard  
 * library function strtok.  It is contrived to circumvent the  
 * problem of returning NULL strings for fields delimited by  
 * consecutive delimiters -- as it must for blank delimiters.    
 * This version returns for consecutive delimiters a non-null   
 * but zero length string. NULL is reserved for end-of-record.  
 *  
 *----------------------------------------------------------------  
 * Calling parameters:  
 *  
 * Parameters:  
 *     inbuf  - First call - pointer to input buffer to be parsed.  
 *     string - delimiter string  
 *----------------------------------------------------------------  
 */  
  
char *strtok_x (char *inbuf, char *string)  
{  
  static char *ptr, *last_ptr, *temp_ptr;  
  
  if (inbuf != NULL) {  
    last_ptr = inbuf;  
  }  
  if (last_ptr == NULL) return NULL;  
  ptr = strstr (last_ptr, string);  
  if (ptr != NULL) {  
    *ptr = '\x00';  
    temp_ptr = last_ptr;  
    last_ptr = &ptr[strlen(string)];  
  } else if (strlen(last_ptr) > 0) {  
    temp_ptr = last_ptr;  
    last_ptr = &last_ptr[strlen(last_ptr)];  
  } else {  
    last_ptr = NULL;  
    temp_ptr = NULL;  
  }  
  return temp_ptr;  
}      
  
  
/*----------------------------------------------------------------  
 *  
 * Character function extract_dir extracts the VAX or Linux directory   
 * name from the file name.  
 *  
 *----------------------------------------------------------------  
 * Calling parameters:  
 *  
 * Parameters:  
 *     file  - file name including directory  
 *----------------------------------------------------------------  
 */  
  
char *extract_dir (char *filename)  
{  
  int i, len = strlen (filename);  
  char *ptr;  
  
  i = len-1;  
  while (i > 0 && filename[i] != ']' &&   
                  filename[i] != '/' &&   
                  filename[i] != '\\') i--;  
  if (filename[i] == ']' ||   
      filename[i] == '/' ||   
      filename[i] == '\\') {  
    ptr = &filename[++i];  
  } else {  
    ptr = filename;  
  }  
  return ptr;  
}      
  
  
/*----------------------------------------------------------------  
 *  
 * Integer function analyze_dvdq sorts the bus_dvdq entities  
 * by three fields: max/min, last/first, and 2nd_last/first and  
 * prints their summaries  
 *  
 *----------------------------------------------------------------  
 * Calling parameters:  
 *  
 * Parameters:  
 *----------------------------------------------------------------  
 */  
  
int analyze_dvdq (int critical_index, int type, MATRIX *pvdata)  
{  
  int i, j, num_dvdq, first, last, second_last, min, max, j1, j2, j3;  
  double dvdq, dvdq_min, dvdq_max;  
  char blank[] = "            ";  
  
/* Load dvdq data */  
  
  if (type == 1) {  
    fprintf (caselog_fp, "\n Summary of Pre-contingency dV/dQ\n\n");  
  } else if (type == 5) {  
    fprintf (caselog_fp, "\n Summary of Post-contingency dV/dQ %s\n\n",  
      critical[critical_index].outage);  
  } else {  
    fprintf (stderr, "Incorrect calling parameter - must be 1 or 5\n");  
    exit (1);  
  }  
  
  num_dvdq = pvdata[type].column_count;  
  
  for (i=2; i<num_dvdq; i++) {  
    bus_dvdq[i].busname = pvdata[type].matrix[2][i];  
    bus_dvdq[i].max_min = 0.0;  
    bus_dvdq[i].last_first = 0.0;  
    bus_dvdq[i].second_last_first = 0.0;  
    first = -1;  
    min = -1;  
    max = -1;  
    dvdq_min = 0.0;  
    dvdq_max = 0.0;  
    if (pvdata[type].column_status[i] == 1) {
      for (j=3; j<pvdata[type-1].row_count; j++) {  
        if (pvdata[type].row_status[j] == 0) {  
        } else {  
          dvdq = atof (pvdata[type].matrix[j][i]);  
          if (first == -1) first = j;  
          if (min == -1) {  
            min = j;  
            dvdq_min = dvdq;  
          } else if (dvdq < dvdq_min) {  
            min = j;  
            dvdq_min = dvdq;  
          }  
          if (max == -1) {  
            max = j;  
            dvdq_max = dvdq;  
          } else if (dvdq > dvdq_max) {  
            max = j;  
            dvdq_max = dvdq;  
          }  
        }  
      }  
      last = -1;  
      second_last = -1;  
      for (j=pvdata[type-1].row_count-1; j>2; j--) {  
        if (pvdata[type].row_status[j] == 0) {  
        } else if (last == -1) {  
          last = j;  
        } else if (second_last == -1) {  
          second_last = j;  
        }  
      }  
      if (dvdq_min != 0.0) {  
        bus_dvdq[i].max_min = dvdq_max / dvdq_min;  
      } else {  
        bus_dvdq[i].max_min = 0.0;  
      }  
    }
    if (first >= 0) {
      dvdq = atof (pvdata[type].matrix[first][i]);
    } else {
      dvdq = 0.0;
    }  
    if (dvdq != 0.0) {  
      bus_dvdq[i].last_first = atof (pvdata[type].matrix[last][i]) / dvdq;  
      bus_dvdq[i].second_last_first = atof (pvdata[type].matrix[second_last][i]) / dvdq;  
    } else {  
      bus_dvdq[i].last_first = 0.0;  
      bus_dvdq[i].second_last_first = 0.0;  
    }   
  }  
  
/* Sort struct bus_dvdq by max_min */  
  
  for (i=0; i<num_dvdq-2; i++) temp_dvdq1[i] = i+2;  
  qsort ((void *) temp_dvdq1, (size_t) num_dvdq-2, sizeof (int),  
                  compare_dvdq1);  
  
/* Sort struct bus_dvdq by last-first */  
  
  for (i=0; i<num_dvdq-2; i++) temp_dvdq2[i] = i+2;  
  qsort ((void *) temp_dvdq2, (size_t) num_dvdq-2, sizeof (int),  
                  compare_dvdq2);  
  
/* Sort struct bus_dvdq by 2nd last-first */  
  
  for (i=0; i<num_dvdq-2; i++) temp_dvdq3[i] = i+2;  
  qsort ((void *) temp_dvdq3, (size_t) num_dvdq-2, sizeof (int),  
                  compare_dvdq3);  
  
/* Write out the summary */  
  
  fprintf (caselog_fp, " Bus            max/min   Bus         last/first   Bus     2nd last/first\n");  
  
  for (i=0; i<num_dvdq-2; i++) {  
    j1 = temp_dvdq1[i];  
    j2 = temp_dvdq2[i];  
    j3 = temp_dvdq3[i];  
    fprintf (caselog_fp, " %-15s %6f   %-15s %6f   %-15s %6f\n",  
      bus_dvdq[j1].busname, bus_dvdq[j1].max_min,   
      bus_dvdq[j2].busname, bus_dvdq[j2].last_first,  
      bus_dvdq[j3].busname, bus_dvdq[j3].second_last_first);  
  }  
  return 0;  
}  
  
/*  
 *----------------------------------------------------------------  
 *  
 * Integer function compare_dvdq1 performs the qsort comparison for  
 * sorting structure bus_dvdq by the field "max_min".  
 *  
 *----------------------------------------------------------------  
 * Calling parameters:  
 *  
 * Parameters:  
 *     op1  - pointer to bus_dvdq[] from pointer in temp_dvdq1[]  
 *     op2  - pointer to bus_dvdq[] from pointer in temp_dvdq1[]  
 *----------------------------------------------------------------  
 */  
  
int compare_dvdq1 ( const void *op1, const void *op2 )  
{  
  const int *p1 = (const int *) op1;  
  const int *p2 = (const int *) op2;  
  int compare;  
  double dvdq1, dvdq2;  
  
  dvdq1 = bus_dvdq[*p1].max_min;  
  dvdq2 = bus_dvdq[*p2].max_min;  
  if (dvdq1 < dvdq2) compare = 1;  
  else if (dvdq1 > dvdq2) compare = -1;  
  else compare = 0;  
  return compare;  
}  
  
/*----------------------------------------------------------------  
 *  
 * Integer function compare_dvdq2 performs the qsort comparison for  
 * sorting structure bus_dvdq by the field "last_first".  
 *  
 *----------------------------------------------------------------  
 * Calling parameters:  
 *  
 * Parameters:  
 *     op1  - pointer to bus_dvdq[] from pointer in temp_dvdq2[]  
 *     op2  - pointer to bus_dvdq[] from pointer in temp_dvdq2[]  
 *----------------------------------------------------------------  
 */  
  
int compare_dvdq2 ( const void *op1, const void *op2 )  
{  
  const int *p1 = (const int *) op1;  
  const int *p2 = (const int *) op2;  
  int compare;  
  double dvdq1, dvdq2;  
  
  dvdq1 = bus_dvdq[*p1].last_first;  
  dvdq2 = bus_dvdq[*p2].last_first;  
  if (dvdq1 < dvdq2) compare = 1;  
  else if (dvdq1 > dvdq2) compare = -1;  
  else compare = 0;  
  return compare;  
}  
  
/*----------------------------------------------------------------  
 *  
 * Integer function compare_dvdq3 performs the qsort comparison for  
 * sorting structure bus_dvdq by the field "second_last_first".  
 *  
 *----------------------------------------------------------------  
 * Calling parameters:  
 *  
 * Parameters:  
 *     op1  - pointer to bus_dvdq[] from pointer in temp_dvdq3[]  
 *     op2  - pointer to bus_dvdq[] from pointer in temp_dvdq3[]  
 *----------------------------------------------------------------  
 */  
  
int compare_dvdq3 ( const void *op1, const void *op2 )  
{  
  const int *p1 = (const int *) op1;  
  const int *p2 = (const int *) op2;  
  int compare;  
  double dvdq1, dvdq2;  
  
  dvdq1 = bus_dvdq[*p1].second_last_first;  
  dvdq2 = bus_dvdq[*p2].second_last_first;  
  if (dvdq1 < dvdq2) compare = 1;  
  else if (dvdq1 > dvdq2) compare = -1;  
  else compare = 0;  
  return compare;  
}  
