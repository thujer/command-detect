//-------------------------------------------------------------------------//
/////////////////////////////////////////////////////////////////////////////
// +---------------------------------------------------------------------+ //
// |                      Detekce prikazu a parametru                    | //
// |                         -------------------                         | //
// |                       Verze 1.23 Build 050523                       | //
// |                         Tomas Hujer (c) 2005                        | //
// +---------------------------------------------------------------------+ //
// |  Detekuje sekvencni prikazy podle tabulky prikazu v parametru       | //
// |  CMD_TestProc(*CMDTab, Get_Char). CMDTab je pointer na tabulku      | //
// |  Get_Char je promenna kde se sekvencne objevuje testovany znak.     | //
// |  Funkce prochazi tabulkou a pri kazdem znaku se rozhoduje podle     | //
// |  kodu a parametru v tabulce jaka bude nasledujici akce, pokud       | //
// |  sekvence znaku prosla cestu v tabulce az k ExitCode, je tato       | //
// |  hodnota vracena. Podle navratove hodnoty mohou byt v hlavnim       | //
// |  programu volany prislusne funkce.                                  | //
// +---------------------------------------------------------------------+ //
//                                                                         //
//   Version upgrade:                                                      //
//    1.20    ...                                                          //
//    1.22   050525   Pridany funkce IsHex a CMD_GetParHex                 //
//    1.23   050609   Pridany cGoNodFar3 (index + 768)                     //
//                            cGoNodFar4 (index + 1024)                    //
//                                                                         //
/////////////////////////////////////////////////////////////////////////////
//-------------------------------------------------------------------------//

//#define DEBUG_MODE

#include "cmd.h"

#define CMD_ParIx CMD_Par[0]     // Asociace s nultym bytem v bufferu
unsigned char CMD_Par[64];         // Buffer parametru
unsigned char CMD_NumOfPar=0;      // Pocet parametru

unsigned char CMD_WaitPar=0;
unsigned char CMD_WaitRet=0;
unsigned char CMD_Space=0;         // Detekce a vynechani mezer

unsigned int CMDTab_ix=0;          // Index v tabulce prikazu

//----------------------------------------------------------------------------------

// Prevede znak na velky
unsigned char UpChar(unsigned char Char) {

    if((Char>='a') && (Char<='z')) {
        return(Char & (255-32));
    } else {
        return(Char);
    }
}


#ifdef DEBUG_MODE
// Vrati nazev makra jako ukazatel na text
unsigned char *ActionText(unsigned char Action)
{ switch(Action)
  { case cGoNod    : return("GoNod");
    case cExitCode : return("ExitCode");
    case cWaitPar  : return("WaitPar");
    case cResetIx  : return("ResetIx");
    case cExitRet  : return("ExitRet");
    case cResetRun : return("RstRun");
    case cGoNodFar : return("GoNodFar");
    case cGoNodFar2: return("GoNodFar2");
    case cGoNodFar3: return("GoNodFar3");
    case cGoNodFar4: return("GoNodFar4");
    case cRptRstIx : return("RptRstIx");
  }
  return("Undefined");
}
#endif


// Je-li znak cislo, pismeno, nebo znamenko vrati 1
unsigned char IsPrint(unsigned char Char)
{ if(((Char >= 'A') && (Char <= 'Z')) ||
     ((Char >= 'a') && (Char <= 'z')) ||
     ((Char >= '0') && (Char <= '9')) ||
     (Char=='+') || (Char=='-') || (Char=='#'))
     return(1);
  else return(0);
}


// Je-li znak cislice vrati 1
unsigned char IsNum(unsigned char Char) {
    if((Char >= '0') && (Char <= '9')) 
        return(1);
    else
        return(0);
}


// Je-li znak hexa cislice vrati 1
unsigned char IsHex(unsigned char Char) { 
    if(((Char >= '0') && (Char <= '9')) ||
     ((UpChar(Char) >= 'A') && (UpChar(Char) <= 'F'))) 
        return(1); 
    else
        return(0);
}


// Vynuluje buffer na ukazateli
void ResetBuffer(unsigned char *Buf, unsigned int Bytes) {
    unsigned char i;
    for(i=0; i<Bytes; i++) {
        Buf[i]=0;
    }
}

// Vrati index v bufferu parametru
//unsigned char CMD_GetParIx()
//{ return(CMD_ParIx);
//}

// Vrati ukazatel na n-ty parametr
unsigned char *CMD_GetPar(unsigned char Index) {
    
    unsigned char i;
    unsigned char ct;

    if(CMD_ParIx >= sizeof(CMD_Par)) {
        CMD_ParIx=(sizeof(CMD_Par)-1);
    }

    i=1;
    ct=0;
    
    while((i < CMD_ParIx) && (ct!=Index)) {
        if(ct==Index) {
            break;
        }

        if(!CMD_Par[i]) {
            ct++;
        }

        i++;
  }

  return(CMD_Par + i);
}


// Vrati cislo zkonvertovane z n-teho parametru
unsigned int CMD_GetParNum(unsigned char Index, unsigned char Omez) {

    unsigned char Num[5];
    unsigned char IxNum,IxPar;
    unsigned int  DestInt;
    unsigned int  Nasob;
    unsigned char Ch;

    for(IxNum=0; IxNum<5; IxNum++) {
        Num[IxNum]=0; // Nuluj buffer
    }

    // Nacti cislice do bufferu
    IxNum=0;
    IxPar=0;

    while(IxNum<5) {
        Ch = CMD_GetPar(Index)[IxPar];
        
        if(!Ch) {
            break;
        }

        if(!IsNum(Ch)) {
            IxPar++;
        } else {
            Num[IxNum]=Ch; IxNum++; IxPar++;
        }
    }

    // IxNum...PocetCislic
    if((Omez) && (Omez<IxNum)) {
        IxNum=Omez;  // Omezeni poctu cislic
    }

    // Vypocitej integer ze znaku v parametru
    Nasob = 1;                          // Nastav pocatecniho nasobitele
    DestInt = 0;                        // Nuluj vysledne cislo
    
    while(IxNum) {
        DestInt += (Num[IxNum-1]-'0') * Nasob; // pocitej
        Nasob *= 10;
        IxNum--;
    }

    return(DestInt);
}


// Vrati hexa cislo zkonvertovane z n-teho parametru
unsigned int CMD_GetParHex(unsigned char Index, unsigned char Omez) {
    
    unsigned char Num[5];
    unsigned char IxNum,IxPar;
    unsigned int  DestInt;
    unsigned int  Nasob;
    unsigned char Ch;

    for(IxNum=0; IxNum<5; IxNum++) {
        Num[IxNum]=0; // Nuluj buffer
    }

    // Nacti cislice do bufferu
    IxNum=0;
    IxPar=0;

    while(IxNum<5) {
        Ch = CMD_GetPar(Index)[IxPar];
        
        if(!Ch) {
            break;
        }

        if(!IsHex(Ch)) {
            IxPar++;
        } else {
            Num[IxNum] = UpChar(Ch);
            IxNum++;
            IxPar++;
        }
  }

  // IxNum...PocetCislic
  if((Omez) && (Omez < IxNum)) {
      IxNum=Omez;  // Omezeni poctu cislic
  }

  // Vypocitej integer ze znaku v parametru
  Nasob=1;                          // Nastav pocatecniho nasobitele
  DestInt=0;                        // Nuluj vysledne cislo

  while(IxNum) {
      if(Num[IxNum-1]<='9') {
          DestInt += (Num[IxNum-1]-'0') * Nasob; // pocitej
      } else {
          DestInt += ((Num[IxNum-1]-'A')+10) * Nasob; // pocitej
      }

      Nasob *= 16;
    IxNum--;
  }

  return(DestInt);
}



// Porovnava znak v Get_Char se znaky v tabulce CMDTab, ridi se makry v tabulce
unsigned char CMD_TestProc(unsigned char *CMDTab, unsigned char Get_Char) {
    
    unsigned char Repeat;
    unsigned char Out;

    #define CMD_Char        CMDTab[CMDTab_ix]
    #define CMD_Yes_Action  CMDTab[CMDTab_ix+1]
    #define CMD_Yes_Par     CMDTab[CMDTab_ix+2]
    #define CMD_No_Action   CMDTab[CMDTab_ix+3]
    #define CMD_No_Par      CMDTab[CMDTab_ix+4]

    Repeat=1;
    Out=0;

    #ifdef DEBUG_MODE
    Serial.printf("Test char %c, adress of CMD table: %p\r\n",Get_Char,CMDTab);
    #endif

    if(!CMD_WaitPar) {                           // Pokud se neceka na parametr
        while(Repeat) {                            // Dokud nastaveno repeat, provadej...
            Repeat=0;

            #ifdef DEBUG_MODE
            Serial.printf("\r\nChar: %c  vs  %c    Index: %d \r\n",Get_Char,CMDTab[CMDTab_ix],CMDTab_ix);
            Serial.printf("ActionYes: %s, Par: %d\r\n",ActionText(CMD_Yes_Action),CMD_Yes_Par);
            Serial.printf("ActionNo : %s, Par: %d\r\n",ActionText(CMD_No_Action),CMD_No_Par);
            #endif

            if(UpChar(Get_Char) == CMDTab[CMDTab_ix]) {        // Zde se znak shoduje
                #ifdef DEBUG_MODE
                Serial.printf("%c",CMDTab[CMDTab_ix]);
                #endif
                
                switch(CMD_Yes_Action) {
                    case cGoNod:
                        CMDTab_ix += CMD_Yes_Par;     // Pripocti hodnotu param.Yes
                        break;

                    case cExitCode:
                        Out=CMD_Yes_Par;
                        CMDTab_ix=0;             

                        break;

                    case cWaitPar:
                        CMD_WaitPar = 1;
                        CMD_WaitRet = CMD_Yes_Par;

                        CMDTab_ix=0;
                        CMD_Space=0;
                        CMD_ParIx=1;

                        ResetBuffer(CMD_Par+1,sizeof(CMD_Par)-1); 
                        break;

                    case cResetIx:   
                        CMDTab_ix=0;                              
                        break;

                    case cExitRet:
                        Out = CMD_Yes_Par;
                        break;

                    case cResetRun:
                        CMDTab_ix=0;
                        CMD_ParIx=0;                 
                        break;

                    case cGoNodFar:
                        CMDTab_ix+=(CMD_Yes_Par + 0x100);         
                        break;

                    case cGoNodFar2: 
                        CMDTab_ix += (CMD_Yes_Par + 0x200);         
                        break;

                    case cGoNodFar3: 
                        CMDTab_ix+=(CMD_Yes_Par + 0x300);
                        break;

                    case cGoNodFar4:
                        CMDTab_ix+=(CMD_Yes_Par + 0x400);         
                        break;

                    case cRptRstIx:  
                        CMDTab_ix=0; CMD_ParIx=0; Repeat=1;       
                        break;
                }

            } else { // Zde se znak neshoduje

                switch(CMD_No_Action) {
                    case cGoNod:        // Pripocti hodnotu par.No
                        CMDTab_ix += CMD_No_Par; Repeat=1;
                        break; 

                    case cExitCode:
                        Out=CMD_No_Par;
                        CMDTab_ix = 0;
                        break;

                    case cWaitPar:
                        CMD_WaitPar = 1;
                        CMD_WaitRet = CMD_No_Par;
                        CMDTab_ix = 0;
                        CMD_Space = 0;
                        CMD_ParIx = 1;
                        ResetBuffer(CMD_Par,sizeof(CMD_Par));
                        break;

                    case cResetIx:
                        CMDTab_ix = 0;
                        CMD_ParIx = 0;
                        Repeat = 0;         
                        break;

                    case cExitRet:
                    Out = CMD_No_Par;                             
                    break;

                    case cResetRun:
                        CMDTab_ix = 0;
                        break;

                    case cGoNodFar:
                        CMDTab_ix += (CMD_No_Par + 0x100);
                        Repeat = 1;  
                        break;

                    case cGoNodFar2:
                        CMDTab_ix += (CMD_No_Par + 0x200); Repeat=1;  
                        break;

                    case cGoNodFar3:
                        CMDTab_ix += (CMD_No_Par + 0x300); Repeat=1;  
                        break;

                    case cGoNodFar4:
                        CMDTab_ix += (CMD_No_Par + 0x400);
                        Repeat=1;
                        break;

                    case cRptRstIx:
                        CMDTab_ix = 0;
                        CMD_ParIx = 0;
                        Repeat = 1;
                        break;
                }
            }
        }   

    } else {  // Ukladej parametr

        switch(Get_Char) {
            case ';':
            case '@':
            case '*':
            case '#':
            case 10:
            case 13:
                if(CMD_ParIx < sizeof(CMD_Par)) {      // Ukonci ukladani par.
                    CMD_Par[CMD_ParIx]=0;
                    CMD_ParIx++;
                    CMD_NumOfPar++;
                }

                CMDTab_ix=0;
                CMD_WaitPar=0;
                CMD_Space=0;
                Out=CMD_WaitRet;
                break;

            case '/':                                      // Dalsi parametr
            case '.':
            case ':':
            case ',':
            case ' ':
                if(CMD_ParIx < sizeof(CMD_Par)) {   // Pokud je jeste misto v bufferu
                    if(CMD_ParIx > 1) {               // a index je vetsi nez 1
                        if(!CMD_Space) {                // a znak neni mezera
                            CMD_Par[CMD_ParIx]=0;       // Zapis do bufferu konec x.parametru
                            CMD_ParIx++;                // Dalsi znak
                            CMD_NumOfPar++;             // Inc pocet parametru
                        }
                    } else {
                        CMD_NumOfPar=0;            // jinak nuluj pocet parametru
                    }
                    
                }
                break;

            case '"':
                break;

            default:
                if(CMD_ParIx < sizeof(CMD_Par)) {
                    if(IsPrint(Get_Char)) {
                        CMD_Par[CMD_ParIx] = Get_Char;
                    } else {
                        CMD_Par[CMD_ParIx] = 0;
                        CMDTab_ix = 0;
                        CMD_WaitPar = 0;
                        Out=CMD_WaitRet;
                    }
                    CMD_ParIx++;
                    CMD_Space = 0;
                }
                break;
        }
    }

    return(Out);
}
