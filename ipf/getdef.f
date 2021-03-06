C    @(#)getdef.f	20.10 2/28/00
      subroutine getdef (ntex, deftxt, ndef, defstr, defval, defind,
     1                   nchr, ndefch, defchr)
      integer defind(*), ndefch(*), error
      real defval(*)
      character deftxt(*)*(*), defstr(*)*(*), defchr(*)*(*)
 
      include 'ipfinc/parametr.inc'
 
      include 'ipfinc/anlys.inc'
      include 'ipfinc/arcntl.inc'
      include 'ipfinc/area.inc'
      include 'ipfinc/basval.inc'
      include 'ipfinc/blank.inc'
      include 'ipfinc/branch.inc'
      include 'ipfinc/bus.inc'
      include 'ipfinc/busanl.inc'
      include 'ipfinc/lfiles.inc'
      include 'ipfinc/losanl.inc'
      include 'ipfinc/prt.inc'
      include 'ipfinc/usranl.inc'
      include 'ipfinc/ownhash.inc'
 
      integer find_bus, first, findex, ptr, itor
      character word(50)*60, zn * 2, deftyp * 60, capital * 60, 
     1          tempc * 120, id * 1
      real rtoi
      equivalence (itor, rtoi)
      external find_bus
 
C     Define Dictionary
C
C     > DEFINE_TYPE OWNER_LOSS
C     LET o1 = <owner1>
C     LET o2 = <owner2>
C     > DEFINE_TYPE AREA_LOSS
C     LET a1 = <area1>
C     LET a2 = <area2>
C     > DEFINE_TYPE ZONE_LOSS
C     LET z1 = <zone1>
C     LET z2 = <zone2>
C     > DEFINE_TYPE SYSTEM_LOSS
C     LET S = system_loss
C     > DEFINE_TYPE OLD_BASE
C     LET A = FILE_NAME
C     LET B = FILE_DATE
C     LET C = FILE_TIME
C     LET D = FILE_DESCRIPTION
C     LET E = PF_VERSION
C     > DEFINE_TYPE BRANCH_P
C     let p1 = Bus1,base1,[*],bus2,base2,[*],id
C     let p2 = Bus3,base3,[*],bus3,base4,[*],id
C     ...
C     let pn = Busm,basem,[*],busn,basen,[*],id (Branch Flow in MW at
C                                            optional metering point)
C     > DEFINE_TYPE BRANCH_Q
C     let q1 = Bus1,base1,[*],bus2,base2,[*],id
C     let q2 = Bus3,base3,[*],bus3,base4,[*],id
C     ...
C     let qn = Busm,basem,[*],busn,basen,[*],id (Branch Flow in MVAR at
C                                             optional metering point)
C     > DEFINE_TYPE INTERTIE_P
C     let i1 = Area_1,Area_2
C     let i2 = area_3,area_4
C     ...
C     let in = area_m,area_n         (Interchange flow in MW)
C
C     > DEFINE_TYPE INTERTIE_Q
C     let j1 = Area_1,Area_2
C     let j2 = area_3,area_4
C     ...
C     let jn = area_m,area_n         (Interchange flow in MVAR)
C
C     > DEFINE_TYPE INTERTIE_P_SCHEDULED
C     let i1 = Area_1,Area_2
C     let i2 = area_3,area_4
C     ...
C     let in = area_m,area_n         (Scheduled Interchange flow in MW)
C
C     > DEFINE_TYPE FUNCTION
C     LET i = h + g + k
C
C     > DEFINE_TYPE BUS_INDEX
C     LET b1 = Bus1,base1
C     LET b2 = Bus2,base2
C     ...
C     LET bn = Busn,basen
C
C     > DEFINE_TYPE LINE_INDEX or DEFINE_TYPE BRANCH_INDEX
C     let l1 = Bus1,base1,bus2,base2,id
C     let l2 = Bus3,base3,bus4,base4,id
C     ...
C     let lp = Busp,basep,busq,baseq,id
C
C     > DEFINE_TYPE INTERTIE_INDEX
C     let i1 = area1,area2
C     let i2 = area3,area4
C     ...
C     let ip = areap,areaq
C
C     > DEFINE_TYPE AREA_INDEX
C     let a1 = area1
C     let a2 = area2
C     ...
C     let ap = areap
C
C     > DEFINE_TYPE ZONE_INDEX
C     let z1 = zone1
C     let z2 = zone2
C     ...
C     let zp = zonep
C
C     > DEFINE_TYPE TRANSFER_INDEX
C     let z1 = <bus1, base1, bus2, base2>
C     let z2 = <bus3, base3, bus4, base4>
C     ...
C     let zp = <busm, basem, busn, basen>
C
      ndef = 0
      nchr = 0
      deftyp = ' '
      do ib = 1, ntex
         call uscan(deftxt(ib),word,nwrd,char(0),'= ,<>()'//char(9))
         do i = 1, nwrd
            word(i) = capital (word(i))
         enddo
         first = 1
 
         if (deftxt(ib)(1:1) .eq. '>') then
            deftyp = word(2)
C
C           Eliminate underscores "_" from DEFTYP.
C
  101       j = index (deftyp, '_')
            if (j .gt. 0) then
               deftyp(j:) = deftyp(j+1:)
               go to 101
            endif
            lc = lastch(deftyp)
C 
C           Search for valid types. 
C 
            if (index ('OWNERLOSSES',deftyp(1:lc)) .ne. 0 .or.
     1          index ('ZONELOSSES',deftyp(1:lc)) .ne. 0 .or.
     2          index ('SYSTEMLOSSES',deftyp(1:lc)) .ne. 0 .or.
     3          index ('AREALOSSES',deftyp(1:lc)) .ne. 0) then
               if (nwrd .ge. 3) then
                  first = 3
                  if (word(first) .eq. 'LET') then
                     go to 130
                  else
                     jc = lastch (word(3))
                     write (errbuf(1), 102)
  102                format (' "LET" missing on following record ')
                     write (errbuf(2),160 ) deftxt(ib)(1:80)
                     call prterx ('W', 2)
                  endif
               else
               endif
            else if (index ('BRANCHP',deftyp(1:lc)) .ne. 0 .or.
     1               index ('BRANCHQ',deftyp(1:lc)) .ne. 0) then
               if (nwrd .ge. 3) then
                  first = 3
                  if (word(first) .eq. 'LET') then
                     go to 130
                  else
                     jc = lastch (word(3))
                     write (errbuf(1), 102)
                     write (errbuf(2),160 ) deftxt(ib)(1:80)
                     call prterx ('W', 2)
                  endif
               endif
            else if (index ('INTERTIEP',deftyp(1:lc)) .ne. 0 .or.
     1               index ('INTERTIEQ',deftyp(1:lc)) .ne. 0 .or.
     2               index ('INTERTIEPSCHEDULED',deftyp(1:lc)) .ne. 0)
     3         then
               if (nwrd .ge. 3) then
                  first = 3
                  if (word(first) .eq. 'LET') then
                     go to 130
                  else
                     jc = lastch (word(3))
                     write (errbuf(1), 102)
                     write (errbuf(2),160 ) deftxt(ib)(1:80)
                     call prterx ('W', 2)
                  endif
               endif
            else if (index ('FUNCTION',deftyp(1:lc)) .ne. 0 .or.
     1               index ('LINEINDEX',deftyp(1:lc)) .ne. 0 .or.
     1               index ('BRANCHINDEX',deftyp(1:lc)) .ne. 0 .or.
     2               index ('BUSINDEX',deftyp(1:lc)) .ne. 0 .or.
     3               index ('INTERTIEINDEX',deftyp(1:lc)) .ne. 0 .or.
     4               index ('ZONEINDEX',deftyp(1:lc)) .ne. 0 .or.
     5               index ('OWNERINDEX',deftyp(1:lc)) .ne. 0 .or.
     6               index ('AREAINDEX',deftyp(1:lc)) .ne. 0 .or.
     6               index ('TRANSFERINDEX',deftyp(1:lc)) .ne. 0) then
               if (nwrd .ge. 3) then
                  first = 3
                  if (word(first) .eq. 'LET') then
                     go to 130
                  else
                     jc = lastch (word(3))
                     write (errbuf(1),102 ) word(3)(1:jc)
                     write (errbuf(2),160 ) deftxt(ib)(1:80)
                     call prterx ('W', 2)
                  endif
               endif
            else if (index ('OLDBASE',deftyp(1:lc)) .ne. 0) then
               if (nwrd .ge. 3) then
                  first = 3
                  if (word(first) .eq. 'LET') then
                     go to 130
                  else
                     jc = lastch (word(3))
                     write (errbuf(1),102 ) word(3)(1:jc)
                     write (errbuf(2),160 ) deftxt(ib)(1:80)
                     call prterx ('W', 2)
                  endif
               endif
            else
 
               write (errbuf(1),120) deftyp(1:lc)
  120          format (' > DEFINE_TYPE (', a, ') not recognized.')
               write (errbuf(2),160) deftxt(ib)(1:80)
               call prterx ('W',2)
C    
C              Set flag to reject all "LET" records until a new  
C              > DEFINE_TYPE command is encountered.  
C  
               deftyp = ' '
               lc = 1
            endif
 
         endif
 
  130    if (deftyp .eq. ' ') then
C          
C           Reject records until > DEFINE_TYPE encountered.  
C 
            write (errbuf(1),120) deftxt(ib)(1:60)
  140       format (' > USER_ANALYSIS text rejected: (', a, ')')
            call prterx ('W',1)
 
         else if (word(first) .eq. 'LET') then
C  
C           > DEFINE_TYPE OWNER_LOSS, LET o1 = <owner1>, o2 = <owner2> 
C  
            if (index ('OWNERLOSSES',deftyp(1:lc)) .ne. 0) then
C
C              Build ownership loss tables if not already built
C
               call updzon()
 
               iw = first + 1
               do while (iw .le. nwrd - 1)
                  if (word(iw) .eq. 'LET') then
                     iw = iw + 1
                  else
C    
C                    Check for duplicate definitions.   
C       
                     do i = 1, ndef
                        if (defstr(i) .eq. word(iw)) then
                           jc = lastch (word(iw))
                           write (errbuf(1),150 ) word(iw)(1:jc)
  150                      format (' Duplicate definition (', a,
     1                        '), symbol ignored on following record.')
                           write (errbuf(2),160 ) deftxt(ib)(1:80)
  160                      format (' > USER_ANALYSIS text: (', a, ')')
                           call prterx ('W', 2)
                           go to 210
                        endif
                     enddo
                     ndef = ndef + 1
                     defstr(ndef) = word(iw)
                     defval(ndef) = 0.0
                     defind(ndef) = 0
                     ndefch(ndef) = 0
  
                     do j = 1, jowner
                        if (ownnam(j) .eq. word(iw+1)) then
                           defval(ndef) = defval(ndef) + ownlos(1,j)
                           if (usrdbg(1) .gt. 0) then
                              write (dbug,180) defstr(ndef), ownnam(j),
     1                           ownlos(1,j)
  180                         format (' *DEFINE symbol ', a, ' owner ',
     &                            a, ' value ', f8.1)
                           endif
                           go to 210
                        endif
                     enddo
 
                     jc = lastch (word(iw+1))
                     write (errbuf(1),200 ) word(iw+1)(1:jc)
  200                format (' Ownership (', a,
     1') is not in base and is ignored on the following record.')
                     write (errbuf(2),160) deftxt(ib)(1:80)
                     call prterx ('W', 2)

  210                iw = iw + 2

                  endif 
               enddo
 
            else if (index ('ZONELOSSES',deftyp(1:lc)) .ne. 0) then
 
               iw = first + 1
               do while (iw .le. nwrd - 1)
                  if (word(iw) .eq. 'LET') then
                     iw = iw + 1
                  else
C      
C                    Check for duplicate definitions.   
C   
                     do i = 1, ndef
                        if (defstr(i) .eq. word(iw)) then
                           jc = lastch (word(iw))
                           write (errbuf(1),150 ) word(iw)(1:jc)
                           write (errbuf(2),160 ) deftxt(ib)(1:80)
                           call prterx ('W', 2)
                           go to 260
                        endif
                     enddo
                     ndef = ndef + 1
                     defstr(ndef) = word(iw)
                     defval(ndef) = 0.0
                     defind(ndef) = 0
                     ndefch(ndef) = 0
 
                     do j = 1, nztot
                        if (acznam(j) .eq. word(iw+1)) then
                           defval(ndef) = defval(ndef) + zsum(5,j)
                           if (usrdbg(1) .gt. 0) then
                              write (dbug,230) defstr(ndef), acznam(j),
     1                           zsum(5,j)
  230                         format (' *DEFINE symbol ', a, ' zone ', 
     &                           a, ' value ', f8.1)
                           endif
                           go to 260
                        endif
                     enddo
 
                     jc = lastch (word(iw+1))
                     write (errbuf(1),250 ) word(iw+1)(1:jc)
  250                format (' Zone (', a,') is not in base ',
     &                  'and is ignored on the following record.')
                     write (errbuf(2),160 ) deftxt(ib)(1:80)
                     call prterx ('W', 2)

  260                iw = iw + 2

                  endif
               enddo
C           
C              Test for SYSTEM_LOSSES 
C           
            else if (index ('SYSTEMLOSSES',deftyp(1:lc)) .ne. 0) then
C
C              Build ownership loss tables if not already built
C
               call updzon()
C       
C              Check for duplicate definitions. 
C     
               iw = first + 1
               do i = 1, ndef
                  if (defstr(i) .eq. word(iw)) then
                     jc = lastch (word(iw))
                     write (errbuf(1),150 ) word(iw)(1:jc)
                     write (errbuf(2),160 ) deftxt(ib)(1:80)
                     call prterx ('W', 2)
                     go to 1000
                  endif
               enddo
               ndef = ndef + 1
               defstr(ndef) = word(iw)
               defval(ndef) = 0.0
               defind(ndef) = 0
               ndefch(ndef) = 0
 
               do j = 1, jowner
                  defval(ndef) = defval(ndef) + ownlos(1,j)
                  if (usrdbg(1) .gt. 0) then
                     write (dbug,280) defstr(ndef), ownnam(j),
     1                  ownlos(1,j)
  280                format (' *DEFINE symbol ', a, ' system ', a,
     1                  ' value ', f8.1)
                  endif
               enddo
 
               if (usrdbg(1) .gt. 0) then
                  write (dbug,300) defstr(ndef), defval(ndef)
  300             format (' *DEFINE symbol ', a, ' TOTAL ', f10.1)
               endif
C  
C              Test for AREA_LOSSES  
C      
            else if (index ('AREALOSSES',deftyp(1:lc)) .ne. 0) then
 
               iw = first + 1
               do while (iw .le. nwrd - 1)
                  if (word(iw) .eq. 'LET') then
                     iw = iw + 1
                  else
C       
C                    Check for duplicate definitions.        
C  
                     do 310 i = 1, ndef
                        if (defstr(i) .eq. word(iw)) then
                           jc = lastch (word(iw))
                           write (errbuf(1),150 ) word(iw)(1:jc)
                           write (errbuf(2),160 ) deftxt(ib)(1:80)
                           call prterx ('W', 2)
                           go to 370
                        endif
  310                continue
                     ndef = ndef + 1
                     defstr(ndef) = word(iw)
                     defval(ndef) = 0.0
                     defind(ndef) = 0
                     ndefch(ndef) = 0
 
                     do j = 1, ntotc
                        if (arcnam(j) .eq. word(iw+1)) then
                           do k = 1, MAXCAZ
                              zn = arczns(k,j)
                              if (j .eq. 1 .or. zn .ne. ' ') then
                                 do l = 1, nztot
                                    if (acznam(l) .eq. zn) then
                                       defval(ndef) = defval(ndef) +
     1                                    zsum(5,l)
                                       if (usrdbg(1) .gt. 0) then
                                          write (dbug,320) defstr(ndef),
     1                                       acznam(j), zn, zsum(5,l)
  320                                     format (' *DEFINE symbol ', a,
     1                                       ' area ', a, ' zone ', a,
     2                                       ' value ', f8.1)
                                       endif
                                       go to 340
                                    endif
                                 enddo
                              endif
  340                         continue
                           enddo
                           go to 370
                        endif
                     enddo
 
                     jc = lastch (word(iw+1))
                     write (errbuf(1),360 ) word(iw+1)(1:jc)
  360                format (' Area (', a,') is not in base and ',
     &                       'is ignored on the following record.')
                     write (errbuf(2),160 ) deftxt(ib)(1:80)
                     call prterx ('W', 2)
 
  370                iw = iw + 2

                  endif
               enddo
C        
C              Test for BRANCH_P or BRANCH_Q.
C        
            else if (index ('BRANCHP',deftyp(1:lc)) .ne. 0 .or.
     2               index ('BRANCHQ',deftyp(1:lc)) .ne. 0) then
 
               iw = first + 1
C                                                                      *
C              Check for duplicate definitions.                        *
C                                                                      *
               do i = 1, ndef
                  if (defstr(i) .eq. word(iw)) then
                     jc = lastch (word(iw))
                     write (errbuf(1),150 ) word(iw)(1:jc)
                     write (errbuf(2),160 ) deftxt(ib)(1:80)
                     call prterx ('W', 2)
                     go to 1000
                  endif
               enddo
               ndef = ndef + 1
               defstr(ndef) = word(iw)
               defval(ndef) = 0.0
               defind(ndef) = 0
               ndefch(ndef) = 0
               tempc = deftxt(ib)
C                                                                      *
C              Test for concatenation of bus names and base KV.
C              If WORD(IW+1) or WORD(IW+3) > 8 characters, insert a
C              blank spacer and rescan.
C                                                                      *
               if (lastch(word(iw+1)) .gt. 8) then
                  do j = nwrd, iw+2, -1
                     word(j+1) = word(j)
                  enddo
                  nwrd = nwrd + 1
                  word(iw+2) = word(iw+1)(9:)
                  word(iw+1) = word(iw+1)(1:8)
               endif
               if (lastch(word(iw+3)) .gt. 8) then
                  do j = nwrd, iw+4, -1
                     word(j+1) = word(j)
                  enddo
                  nwrd = nwrd + 1
                  word(iw+4) = word(iw+3)(9:)
                  word(iw+3) = word(iw+3)(1:8)
               endif
 
               meter = 1
C                                                                      *
C              Test for specified metering point. Metering point       *
C              at bus1 if "*" follows base1; at bus2 if "*" follows    *
C              base2.                                                  *
C                                                                      *
               if (index (word(iw+2),'*') .ne. 0) then
                  meter = 1
                  i = index (word(iw+2), '*')
                  l = lastch (word(iw+2))
C
C                 Partition WORD(IW+2) in accordance with the relative
C                 position of "*".
C
                  if (i .eq. l) then
                     word(iw+2)(i:i) = ' '
                  else if (i .gt. 1) then
                     do j = nwrd, iw+3, -1
                        word(j+1) = word(j)
                     enddo
                     nwrd = nwrd + 1
                     word(iw+3) = word(iw+2)(i+1:)
                     word(iw+2) = word(iw+2)(1:i-1)
                  else
                     word(iw+2) = word(iw+2)(2:)
                  endif
 
               else if (index (word(iw+4),'*') .ne. 0) then
 
                  meter = 2
                  i = index (word(iw+4), '*')
                  l = lastch (word(iw+4))
C
C                 Partition WORD(IW+4) in accordance with the relative
C                 position of "*".
C
                  if (i .eq. l) then
                     word(iw+4)(i:i) = ' '
                  else if (i .gt. 1) then
                     do j = nwrd, iw+5, -1
                        word(j+1) = word(j)
                     enddo
                     nwrd = nwrd + 1
                     word(iw+5) = word(iw+4)(i+1:)
                     word(iw+4) = word(iw+4)(1:i-1)
                  else
                     word(iw+4) = word(iw+4)(2:)
                  endif
               endif
 
               bs1 = ftn_atof (word(iw+2))
               k1 = find_bus (word(iw+1),bs1)
               if (k1 .le. 0) then
                  jc = lastch (word(iw+1))
                  write (errbuf(1),410 ) word(iw+1)(1:jc), bs1
  410             format (' Bus ', a8, f6.1, ' is not in system.')
                  write (errbuf(2),160 ) deftxt(ib)(1:80)
                  call prterx ('W', 2)
               else
                  bs2 = ftn_atof (word(iw+4))
                  k2 = find_bus (word(iw+3),bs2)
                  if (k2 .le. 0) then
                     jc = lastch (word(iw+3))
                     write (errbuf(1),410 ) word(iw+3)(1:jc), bs2
                     write (errbuf(2),160 ) deftxt(ib)(1:80)
                     call prterx ('W', 2)
                  else
                     if (iw+5 .le. nwrd) then
                        id = word(iw+5)
                     else
                        id = ' '
                     endif
                     ptr = numbrn (k1, k2, id, 0)
                     if (ptr .gt. 0) then
                        if (brtype(ptr) .eq. 4) ptr = brnch_nxt(ptr)
                        call getlfo ( ptr, meter, pin, qin )
                        if (index ('BRANCHP',deftyp(1:lc)) .ne. 0) then
                           defval(ndef) = defval(ndef) + pin
                        else
                           defval(ndef) = defval(ndef) + qin
                        endif
                        if (usrdbg(1) .gt. 0) then
                           write (dbug,420) defstr(ndef), bus(k1),
     1                        base(k1), bus(k2), base(k2), pin * factor
  420                      format (' *DEFINE symbol ', a, ' branch ',
     1                        a8, f7.1, 2x, a8, f7.1, ' value ', f8.1)
                        endif
                     else
                        ptr = numbrn (k1, k2, '*', 0)
                        if (ptr .le. 0) then
                           write (errbuf(1),430 ) bus(k1), bs1,
     1                        bus(k2), bs2, id
  430                      format (' Branch ', a8, f6.1, 1x, a8, f6.1,
     1                        1a1, ' has incorrect parallel ID.')
                           write (errbuf(2),160 ) deftxt(ib)(1:80)
                           call prterx ('W', 2)
                        else
                           write (errbuf(1),440 ) bus(k1), bs1,
     1                        bus(k2), bs2, id
  440                      format (' Branch ', a8, f6.1, 1x, a8, f6.1,
     1                        1a1, ' is not in system.')
                           write (errbuf(2),160 ) deftxt(ib)(1:80)
                           call prterx ('W', 2)
                        endif
                     endif
                  endif
               endif
C                                                                      *
C              Test for INTERTIE_P or INTERTIE_Q                       *
C                                                                      *
            else if (index ('INTERTIEP',deftyp(1:lc)) .ne. 0 .or.
     1               index ('INTERTIEQ',deftyp(1:lc)) .ne. 0) then
 
               iw = first + 1
C                                                                      *
C              Check for duplicate definitions.                        *
C                                                                      *
               do i = 1, ndef
                  if (defstr(i) .eq. word(iw)) then
                     jc = lastch (word(iw))
                     write (errbuf(1),150 ) word(iw)(1:jc)
                     write (errbuf(2),160 ) deftxt(ib)(1:80)
                     call prterx ('W', 2)
                     go to 1000
                  endif
               enddo
               ndef = ndef + 1
               defstr(ndef) = word(iw)
               defval(ndef) = 0.0
               defind(ndef) = 0
               ndefch(ndef) = 0
 
               do ka1 = 1, ntotc
                  if (arcnam(ka1) .eq. word(iw+1)) go to 480
               enddo
               jc = lastch (word(iw+1))
               write (errbuf(1),470 ) word(iw+1)(1:jc)
  470          format (' Area ', a, ' is not in interchange system ')
               write (errbuf(2),160 ) deftxt(ib)(1:80)
               call prterx ('W', 2)
               ka1 = 0
  480          do ka2 = 1, ntotc
                  if (arcnam(ka2) .eq. word(iw+2)) go to 500
               enddo
               jc = lastch (word(iw+2))
               write (errbuf(1),470 ) word(iw+2)(1:jc)
               write (errbuf(2),160 ) deftxt(ib)(1:80)
               call prterx ('W', 2)
               ka2 = 0
  500          if (ka1 .ne. 0 .and. ka2 .ne. 0) then
                  do jt = 1, jtie
 
                     k1 = tie(1,jt)
                     k2 = tie(7,jt)
 
                     if (ka1 .eq. tie(2,jt) .and. ka2 .eq.
     1                  tie(8,jt)) then
                        factor = 1.0
                     else if (ka1 .eq. tie(8,jt) .and. ka2 .eq.
     1                  tie(2,jt)) then
                        factor = -1.0
                     else
                        go to 520
                     endif
 
                     call gettfo (jt, pin, qin)
                     if (index ('INTERTIEP',deftyp(1:lc)) .ne. 0) then
                        defval(ndef) = defval(ndef) + factor * pin
                     else
                        defval(ndef) = defval(ndef) + factor * qin
                     endif
                     if (usrdbg(1) .ne. 0) then
                        write (dbug,510) defstr(ndef), arcnam(ka1),
     1                     arcnam(ka2), bus(k1), base(k1), bus(k2),
     2                     base(k2), pin
  510                   format (' *DEFINE symbol ', a, ' intertie ',
     1                     a10, 2x, a10, ' branch ', a8, f7.1, 2x, a8,
     2                     f7.1, ' value ', f8.1)
                     endif
  520                continue
                  enddo
               else
                  jc = lastch (word(iw+1))
                  kc = lastch (word(iw+2))
                  write (errbuf(1),530 ) word(iw+1)(1:jc),
     1               word(iw+2)(1:kc)
  530             format (' Intertie ', a10, ' - ', a10,
     1               ' is not in interchange system ')
                  write (errbuf(2),160 ) deftxt(ib)(1:80)
                  call prterx ('W',2)
               endif
 
C                                                                      *
C              Test for INTERTIE_P_SCHEDULED                           *
C                                                                      *
            else if (index ('INTERTIEPSCHEDULED',deftyp(1:lc)) .ne. 0)
     1         then
 
               iw = first + 1
C                                                                      *
C              Check for duplicate definitions.                        *
C                                                                      *
               do i = 1, ndef
                  if (defstr(i) .eq. word(iw)) then
                     jc = lastch (word(iw))
                     write (errbuf(1),150 ) word(iw)(1:jc)
                     write (errbuf(2),160) deftxt(ib)(1:80)
                     call prterx ('W', 2)
                     go to 1000
                  endif
               enddo
 
               ndef = ndef + 1
               defstr(ndef) = word(iw)
               defval(ndef) = 0.0
               defind(ndef) = 0
               ndefch(ndef) = 0
 
               do i = 1, ntotic
                  if (arcint(1,i) .eq. word(iw+1) .and. arcint(2,i)
     1               .eq. word(iw+2)) go to 570
               enddo
 
               jc = lastch (word(iw+1))
               kc = lastch (word(iw+2))
               write (errbuf(1),560) word(iw+1)(1:jc), word(iw+2)(1:kc)
  560          format (' Scheduled intertie ', a, ' to ', a,
     1            ' is not in intertie system ')
               write (errbuf(2),160) deftxt(ib)(1:80)
               call prterx ('W',2)
               go to 1000
 
  570          defval(ndef) = defval(ndef) + arcinp(i)
 
               if (usrdbg(1) .ne. 0) then
                  write (dbug,580) defstr(ndef), arcint(1,i),
     1               arcint(2,i), arcinp(i)
  580             format (' *DEFINE symbol ', a, ' intertie ', a10, 2x,
     1                a10, ' value ', f8.1)
                  write (dbug,300) defstr(ndef), defval(ndef)
               endif
C                                                                      *
C              Test for FUNCTION                                       *
C                                                                      *
            else if (index ('FUNCTION',deftyp(1:lc)) .ne. 0) then
C                                                                      *
C              Check for duplicate definitions.                        *
C                                                                      *
               iw = first + 1
               do i = 1, ndef
                  if (defstr(i) .eq. word(iw)) then
                     jc = lastch (word(iw))
                     write (errbuf(1),150 ) word(iw)(1:jc)
                     write (errbuf(2),160) deftxt(ib)(1:80)
                     call prterx ('W', 2)
                     go to 1000
                  endif
               enddo
               ndef = ndef + 1
               defstr(ndef) = word(iw)
               defval(ndef) = 0.0
               defind(ndef) = 0
               ndefch(ndef) = 0
C                                                                      *
C              Decipher function string                                *
C                                                                      *
               ix = index (deftxt(ib),'=') + 1
               if (ix .ge. lastch(deftxt(ib))) then
                  write (errbuf(1),600)
  600             format (' Missing "=" in function declaration ')
                  write (errbuf(2),160) deftxt(ib)(1:80)
                  call prterx ('W',2)
               else
                  jc = lastch(deftxt(ib))
                  mode = 0    ! Flag evalfn for base case source
                  defval(ndef) = evalfn (deftxt(ib)(ix:jc), ndef,
     1               defstr, defval, defind, error, mode)
                  if (error .ne. 0) then
                     write (errbuf(1),610)
  610                format (' Syntax error in function declaration ')
                     write (errbuf(2),160) deftxt(ib)(1:80)
                     call prterx ('W',2)
                  endif
               endif
               if (usrdbg(1) .ne. 0) then
                  write (dbug,300) defstr(ndef), defval(ndef)
               endif
C                                                                      *
C              Test for LINE_INDEX or BRANCH_INDEX                     *
C                                                                      *
            else if (index ('LINEINDEX',deftyp(1:lc)) .ne. 0 .or.
     &               index ('BRANCHINDEX',deftyp(1:lc)) .ne. 0) then
 
               iw = first + 1
C                                                                      *
C              Check for duplicate definitions.                        *
C                                                                      *
               do i = 1, ndef
                  if (defstr(i) .eq. word(iw)) then
                     jc = lastch (word(iw))
                     write (errbuf(1),150 ) word(iw)(1:jc)
                     write (errbuf(2),160 ) deftxt(ib)(1:80)
                     call prterx ('W', 2)
                     go to 1000
                  endif
               enddo
               ndef = ndef + 1
               defstr(ndef) = word(iw)
               defval(ndef) = 0.0
               defind(ndef) = 0
               ndefch(ndef) = 0
               tempc = deftxt(ib)
C                                                                      *
C              Test for concatenation of bus names and base KV.
C              If WORD(IW+1) or WORD(IW+3) > 8 characters, insert a
C              blank spacer and rescan.
C                                                                      *
               if (lastch(word(iw+1)) .gt. 8) then
                  do j = nwrd, iw+2, -1
                     word(j+1) = word(j)
                  enddo
                  nwrd = nwrd + 1
                  word(iw+2) = word(iw+1)(9:)
                  word(iw+1) = word(iw+1)(1:8)
               endif
               if (lastch(word(iw+3)) .gt. 8) then
                  do j = nwrd, iw+4, -1
                     word(j+1) = word(j)
                  enddo
                  nwrd = nwrd + 1
                  word(iw+4) = word(iw+3)(9:)
                  word(iw+3) = word(iw+3)(1:8)
               endif
 
               bs1 = ftn_atof (word(iw+2))
               k1 = find_bus (word(iw+1),bs1)
               if (k1 .le. 0) then
                  jc = lastch (word(iw+1))
                  write (errbuf(1),410 ) word(iw+1)(1:jc), bs1
                  write (errbuf(2),160 ) deftxt(ib)(1:80)
                  call prterx ('W', 2)
               else
                  bs2 = ftn_atof (word(iw+4))
                  k2 = find_bus (word(iw+3),bs2)
                  if (k2 .le. 0) then
                     jc = lastch (word(iw+3))
                     write (errbuf(1),410 ) word(iw+3)(1:jc), bs2
                     write (errbuf(2),160 ) deftxt(ib)(1:80)
                     call prterx ('W', 2)
                  else
                     if (iw+5 .le. nwrd) then
                        id = word(iw+5)
                        ptr = numbrn (k1, k2, id, 0)
                     else
                        ptr = numbrn (k1, k2, '*', 0)
                     endif
                     if (ptr .gt. 0) then
                        if (brtype(ptr) .eq. 4) ptr = brnch_nxt(ptr)
                        defind(ndef) = 3
c
c                       integer-real conversion averted below!
c
                        itor = ptr
                        defval(ndef) = rtoi
                        if (usrdbg(1) .gt. 0) then
                           write (dbug,630 ) defstr(ndef), bus(k1),
     1                        base(k1), bus(k2), base(k2), ptr
  630                      format (' *DEFINE symbol ', a, ' branch ',
     1                        a8, f7.1, 2x, a8, f7.1, ' index ', i6)
                        endif
                     else
                        ptr = numbrn (k1, k2, '*', 0)
                        if (ptr .le. 0) then
                           write (errbuf(1),430 ) bus(k1), bs1,
     1                        bus(k2), bs2, id
                           write (errbuf(2),160 ) deftxt(ib)(1:80)
                           call prterx ('W', 2)
                        else
                           write (errbuf(1),440 ) bus(k1), bs1,
     1                        bus(k2), bs2, id
                           write (errbuf(2),160 ) deftxt(ib)(1:80)
                           call prterx ('W', 2)
                        endif
                     endif
                  endif
               endif
C                                                                      *
C              Test for BUS_INDEX                                      *
C                                                                      *
            else if (index ('BUSINDEX',deftyp(1:lc)) .ne. 0) then
 
               iw = first + 1
C                                                                      *
C              Check for duplicate definitions.                        *
C                                                                      *
               do i = 1, ndef
                  if (defstr(i) .eq. word(iw)) then
                     jc = lastch (word(iw))
                     write (errbuf(1),150 ) word(iw)(1:jc)
                     write (errbuf(2),160 ) deftxt(ib)(1:80)
                     call prterx ('W', 2)
                     go to 1000
                  endif
               enddo
               ndef = ndef + 1
               defstr(ndef) = word(iw)
               defval(ndef) = 0.0
               defind(ndef) = 0
               ndefch(ndef) = 0
               tempc = deftxt(ib)
C                                                                      *
C              Test for concatenation of bus names and base KV.
C              If WORD(IW+1) > 8 characters, insert a blank spacer
C              and rescan.
C                                                                      *
               if (lastch(word(iw+1)) .gt. 8) then
                  do j = nwrd, iw+2, -1
                     word(j+1) = word(j)
                  enddo
                  nwrd = nwrd + 1
                  word(iw+2) = word(iw+1)(9:)
                  word(iw+1) = word(iw+1)(1:8)
               endif
 
               bs1 = ftn_atof (word(iw+2))
               k1 = find_bus (word(iw+1),bs1)
               if (k1 .le. 0) then
                  jc = lastch (word(iw+1))
                  write (errbuf(1),410 ) word(iw+1)(1:jc), bs1
                  write (errbuf(2),160 ) deftxt(ib)(1:80)
                  call prterx ('W', 2)
               else
                  defind(ndef) = 2
c
c                 integer-real conversion averted below!
c
                  itor = k1
                  defval(ndef) = rtoi
                  if (usrdbg(1) .gt. 0) then
                     write (dbug,650 ) defstr(ndef), bus(k1),
     1                  base(k1), k1
  650                format (' *DEFINE symbol ', a, ' bus ', a8, f7.1,
     1                  ' index ', i6)
                  endif
               endif
C                                                                      *
C           Test for INTERTIE_INDEX                                    *
C                                                                      *
            else if (index ('INTERTIEINDEX',deftyp(1:lc)) .ne. 0) then
 
               iw = first + 1
C                                                                      *
C              Check for duplicate definitions.                        *
C                                                                      *
               do i = 1, ndef
                  if (defstr(i) .eq. word(iw)) then
                     jc = lastch (word(iw))
                     write (errbuf(1),150 ) word(iw)(1:jc)
                     write (errbuf(2),160) deftxt(ib)(1:80)
                     call prterx ('W', 2)
                     go to 1000
                  endif
               enddo
 
               ndef = ndef + 1
               defstr(ndef) = word(iw)
               defval(ndef) = 0.0
               defind(ndef) = 0
               ndefch(ndef) = 0
 
               do i = 1, ntotic
                  if (arcint(1,i) .eq. word(iw+1) .and. arcint(2,i)
     1               .eq. word(iw+2)) then
                     defind(ndef) = 4
c
c                    integer-real conversion averted below!
c
                     itor = i
                     defval(ndef) = rtoi
                     if (usrdbg(1) .gt. 0) then
                        write (dbug,670 ) defstr(ndef), arcint(1,i),
     1                     arcint(2,i), i
  670                   format (' *DEFINE symbol ', a, ' intertie ',
     1                     a10, 1x, a10, ' index ', i6)
                     endif
                     go to 1000
                  endif
               enddo
 
               jc = lastch (word(iw+1))
               kc = lastch (word(iw+2))
               write (errbuf(1),690 ) word(iw+1)(1:jc),
     1            word(iw+2)(1:kc)
  690          format (' Intertie ', a, ' to ', a,
     1            ' is not in intertie system ')
               write (errbuf(2),160) deftxt(ib)(1:80)
               call prterx ('W',2)
C                                                                      *
C           Test for AREA_INDEX                                        *
C                                                                      *
            else if (index ('AREAINDEX',deftyp(1:lc)) .ne. 0) then
 
               iw = first + 1
C                                                                      *
C              Check for duplicate definitions.                        *
C                                                                      *
               do i = 1, ndef
                  if (defstr(i) .eq. word(iw)) then
                     jc = lastch (word(iw))
                     write (errbuf(1),150 ) word(iw)(1:jc)
                     write (errbuf(2),160 ) deftxt(ib)(1:80)
                     call prterx ('W', 2)
                     go to 1000
                  endif
               enddo
               ndef = ndef + 1
               defstr(ndef) = word(iw)
               defval(ndef) = 0.0
               defind(ndef) = 0
               ndefch(ndef) = 0
 
               do i = 1, ntotc
                  if (arcnam(i) .eq. word(iw+1)) then
                     defind(ndef) = 5
c
c                    integer-real conversion averted below!
c
                     itor = i
                     defval(ndef) = rtoi
                     if (usrdbg(1) .gt. 0) then
                        write (dbug,710 ) defstr(ndef), arcnam(i), i
  710                   format (' *DEFINE symbol ', a, ' area ', a10,
     1                     ' index ', i6)
                     endif
                     go to 1000
                  endif
               enddo
 
               jc = lastch (word(iw+1))
               write (errbuf(1),730 ) word(iw+1)(1:jc)
  730          format (' Area ', a, ' is not in system ')
               write (errbuf(2),160) deftxt(ib)(1:80)
               call prterx ('W',2)
C                                                                      *
C           Test for ZONE_INDEX                                        *
C                                                                      *
            else if (index ('ZONEINDEX',deftyp(1:lc)) .ne. 0) then
 
               iw = first + 1
C                                                                      *
C              Check for duplicate definitions.                        *
C                                                                      *
               do i = 1, ndef
                  if (defstr(i) .eq. word(iw)) then
                     jc = lastch (word(iw))
                     write (errbuf(1),150 ) word(iw)(1:jc)
                     write (errbuf(2),160 ) deftxt(ib)(1:80)
                     call prterx ('W', 2)
                     go to 1000
                  endif
               enddo
C
               ndef = ndef + 1
               defstr(ndef) = word(iw)
               defval(ndef) = 0.0
               defind(ndef) = 0
               ndefch(ndef) = 0
C
               do i = 1, nztot
                  if (acznam(i) .eq. word(iw+1)) then
                     defind(ndef) = 6
c
c                    integer-real conversion averted below!
c
                     itor = i
                     defval(ndef) = rtoi
                     if (usrdbg(1) .gt. 0) then
                        write (dbug,750 ) defstr(ndef), acznam(i), i
  750                   format (' *DEFINE symbol ', a, ' zone ', a10,
     1                     ' index ', i6)
                     endif
                     go to 1000
                  endif
               enddo
 
               jc = lastch (word(iw+1))
               write (errbuf(1),770 ) word(iw+1)(1:jc)
  770          format (' Zone ', a, ' is not in system ')
               write (errbuf(2),160) deftxt(ib)(1:80)
               call prterx ('W',2)
C                                                                      *
C           Test for OWNER_INDEX                                       *
C                                                                      *
            else if (index ('OWNERINDEX',deftyp(1:lc)) .ne. 0) then
 
               iw = first + 1
C                                                                      *
C              Check for duplicate definitions.                        *
C                                                                      *
               do i = 1, ndef
                  if (defstr(i) .eq. word(iw)) then
                     jc = lastch (word(iw))
                     write (errbuf(1),150 ) word(iw)(1:jc)
                     write (errbuf(2),160 ) deftxt(ib)(1:80)
                     call prterx ('W', 2)
                     go to 1000
                  endif
               enddo
C
               ndef = ndef + 1
               defstr(ndef) = word(iw)
               defval(ndef) = 0.0
               defind(ndef) = 0
               ndefch(ndef) = 0
C
               do i = 1, numown
                  if (owner_o(i) .eq. word(iw+1)) then
                     defind(ndef) = 7
c
c                    integer-real conversion averted below!
c
                     itor = i
                     defval(ndef) = rtoi
                     if (usrdbg(1) .gt. 0) then
                        write (dbug, 10750) defstr(ndef), owner_o(i), i
10750                   format (' *DEFINE symbol ', a, ' owner ', a3,
     1                     ' index ', i6)
                     endif
                     go to 1000
                  endif
               enddo
 
               jc = lastch (word(iw+1))
               write (errbuf(1), 10770) word(iw+1)(1:jc)
10770          format (' Onwer ', a, ' is not in system ')
               write (errbuf(2),160) deftxt(ib)(1:80)
               call prterx ('W',2)
C                                                                      *
C           Test for TRANSFER_INDEX                                    *
C                                                                      *
            else if (index ('TRANSFERINDEX',deftyp(1:lc)) .ne. 0) then
 
               iw = first + 1
C                                                                      *
C              Check for duplicate definitions.                        *
C                                                                      *
               do i = 1, ndef
                  if (defstr(i) .eq. word(iw)) then
                     jc = lastch (word(iw))
                     write (errbuf(1),150 ) word(iw)(1:jc)
                     write (errbuf(2),160 ) deftxt(ib)(1:80)
                     call prterx ('W', 2)
                     go to 1000
                  endif
               enddo
               ndef = ndef + 1
               defstr(ndef) = word(iw)
               defval(ndef) = 0.0
               defind(ndef) = 0
               ndefch(ndef) = 0
               tempc = deftxt(ib)
C                                                                      *
C              Test for concatenation of bus names and base KV.
C              If WORD(IW+1) or WORD(IW+3) > 8 characters, insert a
C              blank spacer and rescan.
C                                                                      *
               if (lastch(word(iw+1)) .gt. 8) then
                  do j = nwrd, iw+2, -1
                     word(j+1) = word(j)
                  enddo
                  nwrd = nwrd + 1
                  word(iw+2) = word(iw+1)(9:)
                  word(iw+1) = word(iw+1)(1:8)
               endif
               if (lastch(word(iw+3)) .gt. 8) then
                  do j = nwrd, iw+4, -1
                     word(j+1) = word(j)
                  enddo
                  nwrd = nwrd + 1
                  word(iw+4) = word(iw+3)(9:)
                  word(iw+3) = word(iw+3)(1:8)
               endif
 
               bs1 = ftn_atof (word(iw+2))
               k1 = find_bus (word(iw+1),bs1)
               if (k1 .le. 0) then
                  jc = lastch (word(iw+1))
                  write (errbuf(1),410 ) word(iw+1)(1:jc), bs1
                  write (errbuf(2),160 ) deftxt(ib)(1:80)
                  call prterx ('W', 2)
               else
                  bs2 = ftn_atof (word(iw+4))
                  k2 = find_bus (word(iw+3),bs2)
                  if (k2 .le. 0) then
                     jc = lastch (word(iw+3))
                     write (errbuf(1),410 ) word(iw+3)(1:jc), bs2
                     write (errbuf(2),160 ) deftxt(ib)(1:80)
                     call prterx ('W', 2)
                  else
                     defind(ndef) = 8
c
c                    integer-real conversion averted below!
c
                     itor = ipack_2 (k1, k2)
                     defval(ndef) = rtoi
                     if (usrdbg(1) .gt. 0) then
                       write (dbug, 10752) defstr(ndef), bus(k1),
     1                    base(k1), bus(k2), base(k2), defval(ndef)
10752                     format (' *DEFINE symbol ', a, ' branch ',
     1                      a8, f7.1, 2x, a8, f7.1, ' index ', z8)
                     endif
                  endif
               endif
C                                                                      *
C              Test for OLD_BASE                                       *
C                                                                      *
            else if (index ('OLDBASE',deftyp(1:lc)) .ne. 0) then
C                                                                      *
C              Check for duplicate definitions.                        *
C                                                                      *
               iw = first + 1
               do i = 1, ndef
                  if (defstr(i) .eq. word(iw)) then
                     jc = lastch (word(iw))
                     write (errbuf(1),150 ) word(iw)(1:jc)
                     write (errbuf(2),160) deftxt(ib)(1:80)
                     call prterx ('W', 2)
                     go to 1000
                  endif
               enddo
               ndef = ndef + 1
               defstr(ndef) = word(iw)
               defval(ndef) = 0.0
               defind(ndef) = 0
               ndefch(ndef) = 0
 
               if (findex (word(iw+1), 'FILE') .ne. 0) then
                  nchr = nchr + 1
                  ndefch(ndef) = nchr
                  defchr(nchr) = basval(1)
               else if (findex (word(iw+1), 'DISK') .ne. 0) then
                  nchr = nchr + 1
                  ndefch(ndef) = nchr
                  defchr(nchr) = basval(2)
               else if (findex (word(iw+1), 'DIR') .ne. 0) then
                  nchr = nchr + 1
                  ndefch(ndef) = nchr
                  defchr(nchr) = basval(3)
               else if (findex (word(iw+1), 'CASE') .ne. 0) then
                  nchr = nchr + 1
                  ndefch(ndef) = nchr
                  defchr(nchr) = basval(4)
               else if (findex (word(iw+1), 'DATE') .ne. 0) then
                  nchr = nchr + 1
                  ndefch(ndef) = nchr
                  defchr(nchr) = basval(5)
               else if (findex (word(iw+1), 'TIME') .ne. 0) then
                  nchr = nchr + 1
                  ndefch(ndef) = nchr
                  defchr(nchr) = basval(6)
               else if (findex (word(iw+1), 'DESC') .ne. 0) then
                  nchr = nchr + 1
                  ndefch(ndef) = nchr
                  defchr(nchr) = basval(7)
               else if (findex (word(iw+1), 'PFVER') .ne. 0) then
                  nchr = nchr + 1
                  ndefch(ndef) = nchr
                  defchr(nchr) = basval(8)
               else if (findex (word(iw+1), 'USER') .ne. 0) then
                  nchr = nchr + 1
                  ndefch(ndef) = nchr
                  defchr(nchr) = basval(9)
               else
                  jc = lastch (word(iw))
                  lc = lastch (word(iw+1))
                  write (errbuf(1), 774) word(iw)(1:jc),
     1               word(iw+1)(1:lc)
  774             format (' Key for *DEFINE symbol ', a, ' key ', a,
     1               ' is not recognized')
                  write (errbuf(2), 160) deftxt(ib)(1:80)
                  call prterx ('W', 2)
               endif
               if (usrdbg(1) .gt. 0) then
                  write (dbug, 776) defstr(ndef), word(iw+1), nchr,
     1               defchr(nchr)
  776             format (' *DEFINE symbol ', a, ' key ', a8,
     1               ' index ', i4, ' value ', a10)
                  endif
            endif
 
         else if (deftxt(ib)(1:1) .eq. '>') then
         else
 
            write (errbuf(1),732)
  732       format (' / USER_ANALYSIS record not recognized.')
            write (errbuf(2),160) deftxt(ib)(1:80)
            call prterx ('W',2)
 
         endif
 1000    continue
      enddo

      return
      end
