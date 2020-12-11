// Program that construncts 4 different states using the
// MSP432-P401R board and communicates the state over
// a UART port and also allows the communicator to change
// and check the program state.
//
//
// Existing states
// State 1: where both LED's are off
// State 2: where LED 1 is on and LED 2 is off
// State 3: where LED 2 is on and LED 1 is off
// State 4: where both LED's are on
//
//
// Commands that can be used to interact with this program
// 'N' - selects the next state
// 'P' - selects the previous state
// 'C' - returns the current state

#include "msp.h"

#define DELAY_VALUE 10000;

// Stores the current state of the application
uint8_t CurrentState;

// State 1 where both LED's are off
void State_1(void) {
	P1OUT &= (uint8_t) (~(1<<0));
	P2OUT &= (uint8_t) (~(1<<0));
}

// State 2 where LED 1 is on and LED 2 is off
void State_2(void) {
	P1OUT |= (uint8_t) ((1<<0));
	P2OUT &= (uint8_t) (~(1<<0));
}

// State 3 where LED 2 is on and LED 1 is off
void State_3(void) {
	P1OUT &= (uint8_t) (~(1<<0));
	P2OUT |= (uint8_t) ((1<<0));
}

// State 1 where both LED's are on
void State_4(void) {
	P1OUT |= (uint8_t) ((1<<0));
	P2OUT |= (uint8_t) ((1<<0));
}

// Changes the state to previous state
void PrevState(void) {
	if(CurrentState == 0) {
		State_4();
		CurrentState = 3;
	} else if (CurrentState == 1) {
		State_1();
		CurrentState = 0;
	} else if (CurrentState == 2) {
		State_2();
		CurrentState = 1;
	} else if (CurrentState == 3) {
		State_3();
		CurrentState = 2;
	}
}

// Changes the state to next state
void NextState(void) {
	if(CurrentState == 0) {
		State_2();
		CurrentState = 1;
	} else if (CurrentState == 1) {
		State_3();
		CurrentState = 2;
	} else if (CurrentState == 2) {
		State_4();
		CurrentState = 3;
	} else if (CurrentState == 3) {
		State_1();
		CurrentState = 0;
	}
}

// UART interrupt service routine
void EUSCIA0_IRQHandler(void)
{
	if (EUSCI_A0->IFG & EUSCI_A_IFG_RXIFG)
	{
		// Check if the TX buffer is empty first
		while(!(EUSCI_A0->IFG & EUSCI_A_IFG_TXIFG));

		if(EUSCI_A0->RXBUF == 'N') {						// Command 'N' selects the next state
			NextState();
			EUSCI_A0->TXBUF = CurrentState;
		} else if (EUSCI_A0->RXBUF == 'P') { 		// Command 'P' selects the prev state
			PrevState();
			EUSCI_A0->TXBUF = CurrentState;
		} else if (EUSCI_A0->RXBUF == 'C') {		// Command 'C' returns the current state
			EUSCI_A0->TXBUF = CurrentState;
		}
	}
}

// Interupt that manages button clicks
void PORT1_IRQHandler(void) {	
	int i = DELAY_VALUE;
	
	while(i> 0){i--;}
	
	if((P1IFG & (uint8_t)0x02) != 0){
		P1IFG &= (uint8_t) ~0x02;
		if(!(P1IN & (uint8_t)(1<<1))){					// Button 'P1' selects the prev state
			PrevState();
			EUSCI_A0->TXBUF = CurrentState;
		}
	}
	if((P1IFG & (uint8_t) 0x10) != 0){
		P1IFG &= (uint8_t) ~0x10;
		if(!(P1IN & (uint8_t)(1<<4))){					// Button 'P4' selects the next state
			NextState();
			EUSCI_A0->TXBUF = CurrentState;
		}
	}
}

int main(void)
{
    WDT_A->CTL = WDT_A_CTL_PW |             // Stop watchdog timer
            WDT_A_CTL_HOLD;

    CS->KEY = CS_KEY_VAL;                   // Unlock CS module for register access
    CS->CTL0 = 0;                           // Reset tuning parameters
    CS->CTL0 = CS_CTL0_DCORSEL_3;           // Set DCO to 12MHz (nominal, center of 8-16MHz range)
    CS->CTL1 = CS_CTL1_SELA_2 |             // Select ACLK = REFO
            CS_CTL1_SELS_3 |                // SMCLK = DCO
            CS_CTL1_SELM_3;                 // MCLK = DCO
    CS->KEY = 0;                            // Lock CS module from unintended accesses

    // Configure UART pins
    P1->SEL0 |= BIT2 | BIT3;                // set 2-UART pin as secondary function

    // Configure UART
    EUSCI_A0->CTLW0 |= EUSCI_A_CTLW0_SWRST; // Put eUSCI in reset
    EUSCI_A0->CTLW0 = EUSCI_A_CTLW0_SWRST | // Remain eUSCI in reset
            EUSCI_B_CTLW0_SSEL__SMCLK;      // Configure eUSCI clock source for SMCLK
    // Baud Rate calculation
    // 12000000/(16*9600) = 78.125
    // Fractional portion = 0.125
    // User's Guide Table 21-4: UCBRSx = 0x10
    // UCBRFx = int ( (78.125-78)*16) = 2
    EUSCI_A0->BRW = 78;                     // 12000000/16/9600
    EUSCI_A0->MCTLW = (2 << EUSCI_A_MCTLW_BRF_OFS) |
            EUSCI_A_MCTLW_OS16;

    EUSCI_A0->CTLW0 &= ~EUSCI_A_CTLW0_SWRST; // Initialize eUSCI
    EUSCI_A0->IFG &= ~EUSCI_A_IFG_RXIFG;    // Clear eUSCI RX interrupt flag
    EUSCI_A0->IE |= EUSCI_A_IE_RXIE;        // Enable USCI_A0 RX interrupt

    // Enable sleep on exit from ISR
    SCB->SCR |= SCB_SCR_SLEEPONEXIT_Msk;

    // Enable global interrupt
    __enable_irq();

    // Enable eUSCIA0 interrupt in NVIC module
    NVIC->ISER[0] = 1 << ((EUSCIA0_IRQn) & 31);
		
		// Setting up the 'P1' button
		P1SEL0 &= (uint8_t)(~(1<<0));
		P1SEL1 &= (uint8_t)(~(1<<0));
		P1DIR |= (uint8_t)(1<<0);
		P1DS &= (uint8_t)(~(1<<0));
		P1OUT &= (uint8_t)(~(1<<0));
		
		// Setting up the 'P1' buttons
		P2SEL0 &= (uint8_t)(~((1<<0)|(1<<1)|(1<<2)));
		P2SEL1 &= (uint8_t)(~((1<<0)|(1<<1)|(1<<2)));
		P2DIR |= (uint8_t)((1<<0)|(1<<1)|(1<<2));
		P2DS &= (uint8_t)(~((1<<0)|(1<<1)|(1<<2)));
		P2OUT &= (uint8_t)(~((1<<0)|(1<<1)|(1<<2)));
		
		// Setting up the 'P2' LED's
		P1SEL0 &= (uint8_t)(~((1<<4)|(1<<1)));
		P1SEL1 &= (uint8_t)(~((1<<4)|(1<<1)));
		P1DIR &= (uint8_t)(~((1<<4)|(1<<1)));
		P1REN |= (uint8_t)(((1<<4)|(1<<1)));
		P1OUT |= (uint8_t)(((1<<4)|(1<<1)));
		
		// Setting up the 'P1' interupt
		P1IES |= (uint8_t)0x12;
		P1IFG &= (uint8_t)~0x12;
		P1IE |= (uint8_t)0x12;

		NVIC_SetPriority(PORT1_IRQn,2);
		NVIC_ClearPendingIRQ(PORT1_IRQn);
		NVIC_EnableIRQ(PORT1_IRQn);
	
		__ASM("CPSIE I");

    // Enter LPM0
    __sleep();
    __no_operation();                       // For debugger
}
