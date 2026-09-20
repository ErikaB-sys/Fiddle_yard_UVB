# MOTOR  – Prepaire LEFT Flow

Flow of the UART receive state machine.

```mermaid
flowchart TD

     

    B{MOTOR Busy}
    B -->|NO| A[Referenziert ?]
    B -->|YES| G[ Send NAK ]
 
    A[Referenziert ?]   
    A --> |Yes| E[Check RANGE]
    A --> |No| C[Check Steps ]
   
    C --> F{Steps <=200?}
    F -->|Yes| K{CHECK ENDlage L}
    F -->|No| G[ Send NAK ]
    
   
    
 
    
    E --> K{ENDSCHALTER L?}
    K -->|Yes| G[ Send NAK ]
    K -->|No| L[CALC_Profil]
    

    L--> D[Start_Timer]
