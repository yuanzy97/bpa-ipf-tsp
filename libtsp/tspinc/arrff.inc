*BEGIN ARRFF
      DIMENSION ARRAY(40),A(40)
       EQUIVALENCE         (ARRAY(1),  VRMAX), (ARRAY(2),  VRMIN),
     1 (ARRAY(3),   VREF), (ARRAY(4),  ESATX), (ARRAY(5),  CSATX),
     2 (ARRAY(6),    HBR), (ARRAY(7),    CKE), (ARRAY(8),    CKA),
     3 (ARRAY(9),     DR), (ARRAY(10),  CFD1), (ARRAY(11),   CA6),
     4 (ARRAY(12),    DF), (ARRAY(13),   AKF), (ARRAY(14),   HBF),
     5 (ARRAY(15),   HCB), (ARRAY(16),   CKC), (ARRAY(17),    DB),
     6 (ARRAY(18),    DC), (ARRAY(19),   HBE), (ARRAY(20),    DE),
     7 (ARRAY(21),  RCEX), (ARRAY(22),  VRFE), (ARRAY(23),  XCEX),
     8 (ARRAY(24),    VA), (ARRAY(25),    DA), (ARRAY(26),   HBA),
     9 (ARRAY(27),   VCO), (ARRAY(28), VAMAX), (ARRAY(29),   EFD),
     * (ARRAY(30),   CKD), (ARRAY(31), VEMIN), (ARRAY(32), VEMAX),
     * (ARRAY(33), VAMIN), (ARRAY(34),   VE1), (ARRAY(35),   CKB),
     * (ARRAY(36),    A4), (ARRAY(37),    A5), (ARRAY(38),    A6),
     * (ARRAY(39),VFEMAX), (ARRAY(40),   CKH)
      EQUIVALENCE          (ARRAY(13),   CKF)
