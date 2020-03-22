#include <msp430g2553.h>

/**************************************************************
 * Title: DONT-CORRUPT-PLS
 * Version: V0.00
 * Author: nruss
 * Description: Precise Beat follower. Requires manual sync.
 * Comments:    SWE - 105bpm - 570ms
 *              IDC - 126bpm - 476ms
 *************************************************************/
#define TRUE 1
#define FALSE 0
#define MSB 7442
#define LSB (MSB/8)

/**
 * main.c
 */
unsigned char high_octet, high_quartet, high_beat, high_type, beat_type, output;
unsigned short int octet, quartet, beat;
unsigned long int total_beats;

void initClock(void);
void initTimerA(void);
void initGPIO(void);
void sendOutput(unsigned char out_byte, unsigned char state);

void main(void)
{
  WDTCTL = WDTPW | WDTHOLD;   // stop watchdog timer

    initClock();
    initTimerA();
    initGPIO();
    _enable_interrupt();

    beat_type = 1;
    while(1){
        TACCR0 = MSB-1;
        TACCR1 = LSB-1;
        switch (beat_type){
        case 1:
            if(beat>=4){
                if(quartet <=3){
                    output = BIT6;
                    high_type = high_quartet;
                }else{
                    output = BIT7;
                    high_type = high_beat;
                }

            }else{
                output = BIT7;
                high_type = high_beat;
            }

            break;
        default:
            break;
        }

        sendOutput(output, high_type);
    }
}

void initClock(void){
    //Set MCLK = SMCLK = 1MHz
    BCSCTL1 = CALBC1_1MHZ;
    DCOCTL = CALDCO_1MHZ;
}

void initTimerA(void){
    TACCTL0 |= CCIE; // Enable interrupt for CCR0
    TACCTL1 |= CCIE; // Enable interrupt for CCR1
    TACTL = TASSEL_2 + ID_3 + MC_1 + TAIE; //Select SMCLK, SMCLK/8, Up Mode, Interrupt Enable
}

void initGPIO(void){
    //Inputs
    P1REN |= BIT3;  //Enable Pull-Up/Down Resistor
    P1OUT |= BIT3; //Pull-Down @ P1.3
    P1IES |= BIT3;  //interrupt from high to low
    P1IE |= BIT3;   //enable interrupt on BIT3
    P1IFG &= ~BIT3; //clear interrupt flag


    //Outputs
    P1DIR &= 0x00;
    P1DIR |= BIT7 + BIT6 + BIT0;
    P1OUT &=0x00;
}

void sendOutput(unsigned char out_byte, unsigned char state){
    if (state == TRUE)
        out_byte |= BIT0;
    if (state == FALSE)
        out_byte &= 0x00;

    P1OUT = out_byte;
}

//Timer ISR
#pragma vector = TIMER0_A0_VECTOR
__interrupt void Timer_A0(void){
    high_octet = TRUE;
    if(octet>=2){
        octet = 1;
        high_quartet = TRUE;
        if(quartet >= 4){
            quartet = 1;
            high_beat = TRUE;
            total_beats ++;
            if(beat >= 4){
                beat = 1;
            }else
                beat += 1;
        }else
            quartet += 1;
    }else
        octet +=1;
}

#pragma vector = TIMER0_A1_VECTOR
__interrupt void Timer_A1(void){
    switch (TA0IV)
    {

    case 2:
        high_octet = FALSE;
        high_quartet = FALSE;
        high_beat = FALSE;
        break;

    default:
        break;
    }
}

//P1 ISR
#pragma vector = PORT1_VECTOR
__interrupt void P1_ISR(void){
    if((P1IN & BIT3)){
        P1IFG &= ~BIT3;
    }else{
        TACCR0 = 0;
        octet = 1;
        quartet = 1;
        beat = 1;
        total_beats = 1;
    }
}

