///////////////////////////////////////////////////////////////  Hectorkey.H
// Thank's Daniel !
/*      Hector 2HR+
        Victor
        Hector 2HR
        Hector HRX
        Hector MX40c
        Hector MX80c
        Hector 1
        Interact
             
        29/10/2009 Update skeleton to functional machine
                          by yo_fr       (jj.stac@aliceadsl.fr)
               
               => add Keyboard,
               => add color, 
               => add cassette,
               => add sn76477 sound and 1bit sound,
               => add joysticks (stick, pot, fire)
               => add BR/HR switching 
               => add bank switch for HRX
               => add device MX80c and bank switching for the ROM
    Importante note : the keyboard function add been piked from 
                      DChector project : http://dchector.free.fr/ made by DanielCoulom
                      (thank's Daniel)
    TODO : Add the cartridge function,
           Adjust the one shot and A/D timing (sn76477)
*/
static const UINT8 hectorkey[0x100] = {

///////////////////////////////////////// 0x00-0x0f
0xff, //0
0x00, //1= ESC --> Reset machine
0x11, //2=   1 --> 1
0x10, //3=   2 --> 2
0x27, //4=   3 --> 3
0x26, //5=   4 --> 4
0x25, //6=   5 --> 5
0x24, //7=   6 --> 6
0x23, //8=   7 --> 7
0x22, //9=   8 --> 8
0x21, //a=   9 --> 9
0x12, //b=   0 --> 0
0x15, //c=   ) --> - 
0x17, //d=   = --> +
0x04, //e=BACK --> BACK 
0x03, //f= TAB --> TAB

///////////////////////////////////////// 0x10-0x1f
0x31, //0=   A --> A
0x60, //1=   Z --> Z
0x45, //2=   E --> E
0x50, //3=   R --> R
0x66, //4=   T --> T
0x61, //5=   Y --> Y
0x65, //6=   U --> U
0x41, //7=   I --> I
0x53, //8=   O --> O
0x52, //9=   P --> P
0xff, //a=   ^ 
0x35, //b=   $ --> =
0x02, //c= ENT --> RETURN
0x06, //d=CTRL --> CTRL
0x51, //e=   Q --> Q
0x67, //f=   S --> S

///////////////////////////////////////// 0x20-0x2f
0x46, //0=   D --> D
0x44, //1=   F --> F
0x43, //2=   G --> G
0x42, //3=   H --> H
0x40, //4=   J --> J
0x57, //5=   K --> K
0x56, //6=   L --> L
0x55, //7=   M --> M
0x13, //8=   % --> /  
0x08, //9=   ² --> manette 0 ACTION
0x07, //a= MAJ --> SHIFT
0x00, //b=   * --> *
0x63, //c=   W --> W
0x62, //d=   X --> X
0x47, //e=   C --> C
0x64, //f=   V --> V

///////////////////////////////////////// 0x30-0x3f
0x30, //0=   B --> B
0x54, //1=   N --> N
0x16, //2=   , --> ,
0x37, //3=   ; --> ;
0x14, //4=   : --> .
0x33, //5=   ! --> ?
0xff, //6= MAJ
0xff, //7
0xff, //8
0x01, //9=  SP --> SPACE
0x05, //a=LOCK --> LOCK
0xff, //b=  F1
0xff, //c=  F2
0xff, //d=  F3
0xff, //e=  F4
0xff, //f=  F5

///////////////////////////////////////// 0x40-0x4f
0xff, //0
0xff, //1
0xff, //2
0xff, //3
0xff, //4
0xff, //5
0xff, //6
0x74, //7=   7 --> manette 1 GAUCHE
0x77, //8=   8 --> manette 1 BAS
0x75, //9=   9 --> manette 1 DROITE
0x09, //a=   - --> manette 1 ACTION
0x0b, //b=   <- --> pot0-
0x72, //c=   5 --> manette 0 HAUT
0x0a, //d=   <- --> pot0+
0x08, //e=   + --> manette 0 ACTION
0x70, //f=   1 --> manette 0 GAUCHE

///////////////////////////////////////// 0x50-0x5f
0x73, //0=   2 --> manette 0 BAS
0x71, //1=   3 --> manette 0 DROITE
0x0e, //2= INS --> pot1 ++
0x0f, //3=SUPP -->^pot1 --
0xff, //4
0xff, //5
0xff, //6    < -->
0xff, //7
0xff, //8
0xff, //9
0xff, //a
0xff, //b
0xff, //c
0xff, //d
0xff, //e
0xff, //f

///////////////////////////////////////// 0x60-0x6f
0xff, //0=  
0xff, //1
0xff, //2
0xff, //3
0xff, //4
0xff, //5
0xff, //6
0xff, //7
0xff, //8
0xff, //9
0xff, //a
0xff, //b
0xff, //c
0xff, //d
0xff, //e
0xff, //f

///////////////////////////////////////// 0x70-0x7f
0xff, //0
0xff, //1
0xff, //2
0xff, //3
0xff, //4
0xff, //5
0xff, //6
0xff, //7
0xff, //8
0xff, //9
0xff, //a
0xff, //b
0xff, //c
0xff, //d
0xff, //e
0xff, //f

///////////////////////////////////////// 0x80-0x8f
0xff, //0
0xff, //1
0xff, //2
0xff, //3
0xff, //4
0xff, //5
0xff, //6
0xff, //7
0xff, //8
0xff, //9
0xff, //a
0xff, //b
0xff, //c
0xff, //d
0xff, //e
0xff, //f

///////////////////////////////////////// 0x90-0x9f
0xff, //0
0xff, //1
0xff, //2
0xff, //3
0xff, //4
0xff, //5
0xff, //6
0xff, //7
0xff, //8
0xff, //9
0xff, //a
0xff, //b
0x08, //c= ENT --> manette 0 ACTION
0xff, //d= CTRL droit
0xff, //e
0xff, //f

///////////////////////////////////////// 0xa0-0xaf
0xff, //0
0xff, //1
0xff, //2
0xff, //3
0xff, //4
0xff, //5
0xff, //6
0xff, //7
0xff, //8
0xff, //9
0xff, //a
0xff, //b
0xff, //c
0xff, //d
0xff, //e
0xff, //f

///////////////////////////////////////// 0xb0-0xbf
0xff, //0
0xff, //1
0xff, //2
0xff, //3
0xff, //4
0x76, //5=   / --> manette 1 HAUT
0xff, //6
0xff, //7
0xff, //8
0xff, //9
0xff, //a
0xff, //b
0xff, //c
0xff, //d
0xff, //e
0xff, //f

///////////////////////////////////////// 0xc0-0xcf
0xff, //0
0xff, //1
0xff, //2
0xff, //3
0xff, //4
0xff, //5
0xff, //6
0xff, //7=HOME 
0x0d, //8=  UP  
0x0f, //9= PUP 
0xff, //a
0xff, //b
0xff, //c
0xff, //d 
0xff, //e
0xff, //f= END 

///////////////////////////////////////// 0xd0-0xdf
0x0c, //0=DOWN  
0x0e, //1=PDWN 
0xff, //2= INS  
0xff, //3= SUP 
0xff, //4
0xff, //5
0xff, //6
0xff, //7
0xff, //8
0xff, //9
0xff, //a
0xff, //b
0xff, //c
0xff, //d
0xff, //e
0xff, //f

///////////////////////////////////////// 0xe0-0xef
0xff, //0
0xff, //1
0xff, //2
0xff, //3
0xff, //4
0xff, //5
0xff, //6
0xff, //7
0xff, //8
0xff, //9
0xff, //a
0xff, //b
0xff, //c
0xff, //d
0xff, //e
0xff, //f

///////////////////////////////////////// 0xf0-0xff
0xff, //0
0xff, //1
0xff, //2
0xff, //3
0xff, //4
0xff, //5
0xff, //6
0xff, //7
0xff, //8
0xff, //9
0xff, //a
0xff, //b
0xff, //c
0xff, //d
0xff, //e
0xff};//f
