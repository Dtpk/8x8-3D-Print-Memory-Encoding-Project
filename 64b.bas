' ==============================================================================
' MASTER RETRO TWO-WAY TERMINAL (INLINE ALIGNMENT & LEN-GUARD ENGINE)
' ==============================================================================
CLEAR
CLS

DIM SHARED CharList$(1 TO 75)
DIM SHARED CodeList$(1 TO 75)
DIM SHARED TotalCodes AS INTEGER
TotalCodes = 67

' Populate arrays using standard DATA statements
FOR k = 1 TO TotalCodes
    READ CharList$(k), CodeList$(k)
NEXT k

' Main Core Loop
DO
    CLS
    PRINT "Welcome to the Marble Memory System."
    PRINT "1: Encode a word (Text -> Marble Map)"
    PRINT "2: Decode a board (Marble Bits -> Text)"
    PRINT "3: Exit Terminal"
    PRINT
    PRINT "Select option: ";
    INPUT Choice$
    Choice$ = LTRIM$(RTRIM$(Choice$))

    IF Choice$ = "1" THEN
        CLS
        PRINT "=== DIGITAL MARBLE MEMORY ENCODER ==="
        PRINT "Text to parse: ";
        INPUT UserText$

        DIM BitStream$
        BitStream$ = ""
        FOR i = 1 TO LEN(UserText$)
            C$ = MID$(UserText$, i, 1)
            FoundCode$ = ""
            FOR j = 1 TO TotalCodes
                IF CharList$(j) = C$ THEN
                    FoundCode$ = CodeList$(j)
                    EXIT FOR
                END IF
            NEXT j
            IF FoundCode$ = "" THEN
                FOR j = 1 TO TotalCodes
                    IF CharList$(j) = "." THEN FoundCode$ = CodeList$(j)
                NEXT j
            END IF
            BitStream$ = BitStream$ + FoundCode$
        NEXT i

        TotalBits = LEN(BitStream$)
        PRINT
        PRINT "============================================="
        PRINT "STATIC ENCODER OUTPUT FOR: """; UserText$; """"
        PRINT "Total bits used: "; TotalBits; " / 64 maximum"
        PRINT "============================================="

        IF TotalBits > 64 THEN
            PRINT "❌ OVERFLOW! Truncating bitstream..."
            BitStream$ = LEFT$(BitStream$, 64)
        END IF

        PRINT
        PRINT "--- 8x8 MARBLE ARRANGEMENT BLUEPRINT ---"
        PRINT "    0 1 2 3 4 5 6 7"
        PRINT "    -----------------"
        FOR Row = 0 TO 7
            RowString$ = "R" + STR$(Row) + ":  "
            RowString$ = LTRIM$(RowString$)
            IF LEN(RowString$) = 4 THEN RowString$ = RowString$ + " "
            FOR Col = 0 TO 7
                BitIndex = (Row * 8) + Col + 1
                IF BitIndex <= LEN(BitStream$) THEN
                    IF MID$(BitStream$, BitIndex, 1) = "1" THEN
                        RowString$ = RowString$ + "O "
                    ELSE
                        RowString$ = RowString$ + ". "
                    END IF
                ELSE
                    RowString$ = RowString$ + ". "
                END IF
            NEXT Col
            PRINT RowString$
        NEXT Row
        PRINT "    -----------------"
        PRINT
        PRINT "Press [Enter] to return...";
        INPUT "", Dummy$

    ELSEIF Choice$ = "2" THEN
        CLS
        PRINT "=== DIGITAL MARBLE MEMORY DECODER ==="
        PRINT "Type exactly 8 bits per row. Empty line defaults to zeros."
        PRINT "========================================================"
        PRINT "    01234567"
        PRINT "    --------"

        DIM FullBitstream$
        FullBitstream$ = ""

        FOR Row = 0 TO 7
            DIM RowInput$
            DIM ValidRow AS INTEGER
            ValidRow = 0

            DO
                PRINT "R" + LTRIM$(STR$(Row)) + ": ";
                INPUT "", RowInput$
                RowInput$ = LTRIM$(RTRIM$(RowInput$))

                IF RowInput$ = "" THEN RowInput$ = "00000000"

                IF LEN(RowInput$) = 8 THEN
                    ValidRow = 1
                    FullBitstream$ = FullBitstream$ + RowInput$
                ELSE
                    PRINT "⚠️ Error: Length was"; STR$(LEN(RowInput$)); " bits. Re-type row."
                END IF
            LOOP UNTIL ValidRow = 1
        NEXT Row

        PRINT "    --------"
        PRINT
        PRINT "========================================"
        PRINT "DECODED MESSAGE: ";

        DIM CurrentBuffer$
        CurrentBuffer$ = ""
        FOR i = 1 TO LEN(FullBitstream$)
            CurrentBuffer$ = CurrentBuffer$ + MID$(FullBitstream$, i, 1)
            FoundChar$ = ""
            FOR j = 1 TO TotalCodes
                IF CodeList$(j) = CurrentBuffer$ THEN
                    FoundChar$ = CharList$(j)
                    EXIT FOR
                END IF
            NEXT j
            IF FoundChar$ <> "" THEN
                PRINT FoundChar$;
                CurrentBuffer$ = ""
            END IF
        NEXT i
        PRINT
        PRINT "========================================"
        PRINT
        PRINT "Press [Enter] to return...";
        INPUT "", Dummy$

    END IF
LOOP UNTIL Choice$ = "3"
END

' ==============================================================================
' DATA DECK LOADER
' ==============================================================================
DATA " ","000","P","00100000","O","00100001","j","00100010","S","00100011"
DATA "g","001001","m","00101","r","0011","s","0100","?","010100000"
DATA "!","010100001","A","01010001","I","01010010","X","010100110"
DATA "Q","010100111","T","01010100","E","01010101","k","0101011"
DATA "u","01011","n","0110","i","0111","o","1000","3","100100000"
DATA "0","100100001","7","100100010","6","100100011","v","1001001"
DATA "p","100101","c","10011","a","1010","5","101100000","4","101100001"
DATA "x","10110001","K","101100100","J","101100101","9","101100110"
DATA "8","101100111","Y","101101000","V","101101001","-","101101010"
DATA "2","101101011","b","1011011","d","10111","t","1100","y","1101000"
DATA "1","110100100","q","110100101","L","110100110","G","110100111"
DATA "f","110101","l","11011","W","111000000","U","111000001"
DATA "F","111000010","D","111000011","w","1110001","C","111001000"
DATA "B","111001001",".","111001010","R","111001011","H","111001100"
DATA "Z","1110011010","z","1110011011","N","111001110","M","111001111"
DATA "h","11101","e","1111"
