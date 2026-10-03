#include <lpc21xx.h>
#include "KPM_defines.h"
#include "types.h"
#include "defines.h"

/*u32 kpmLUT[4][4]={{1,2,3,4},
										{5,6,7,8},
										{9,10,11,12},
										{13,14,15,16}}; //this for display 1 to 16 num*/

u8 kpmLUT[4][4]={{'7','8','9','A'},
                 {'4','5','6','B'},
                 {'1','2','3','C'},
								 {'c','0','=','D'}};

void Init_KPM(void){
        IODIR1|=15<<ROW0;
}
u32 colscan(void){
        if(((IOPIN1>>COL0)&15)<15){
                        return 0;
        }
        else{
                return 1;
        }
}
u32 rowcheck(void){
        u32 rno;
        for(rno=0;rno<4;rno++){
                //IOPIN1=(IOPIN1&~(15<<ROW0))|(~(1<<rno)&(0X0f)<<ROW0);
								WRITENIBBLE(IOPIN1,ROW0,(~(1<<rno)&0X0f));
                if(colscan()==0)
                                break;
        }
        IOCLR1=15<<ROW0;
        return rno;
}

u32 colcheck(void){
        u32 cno;
        for(cno=0;cno<4;cno++){
                if(((IOPIN1>>(COL0+cno))&1)==0)
                                break;
        }
        return cno;
}

u32 keyscan(void){
        u32 rno,cno,key;
        //wait for switch press
        while(colscan());
        //find the rno
        rno=rowcheck();
        //find the colcheck
        cno=colcheck();
        //get the value from LUT
        key=kpmLUT[rno][cno];
        //wait for switch release
        while(!colscan());
        return key;
        }

u32 readNum(void){
        u8 key;
        u32 sum=0;
        while(1){
                key=keyscan();
                if(key>='0'&&key<='9'){
                        sum=(sum*10)+(key-'0');
                }
                else{
                        break;
                }
        }
                return sum;

}

// Non-Blocking Key Scan Function
u32 keyscan_nb(void) {
    u32 rno, cno, key;
    
    // 1. Check if a key is pressed right now (Instant check)
    // colscan() returns 1 if NO key is pressed, 0 if a key IS pressed
    if (colscan() == 1) {
        return 0; // Return 0 immediately if idle, allowing timers to tick!
    }
    
    // 2. If a key is pressed, find row and column
    rno = rowcheck();
    cno = colcheck();
    
    // 3. Get the character from the lookup table
    key = kpmLUT[rno][cno];
    
    // 4. Wait for switch release (briefly pauses only while user holds the button)
    while (!colscan());
    
    return key;
}
