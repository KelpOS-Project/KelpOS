#include "net_compat.h"

/* Inkarnation des BoredOS-Treibers (wird aus dem BoredOS /net Code gelinkt) */
extern int bored_nic_init(void); 

void kelp_net_subsystem_init(void) {
    /* Hier läuft reiner Kernel-Code ohne Standardbibliothek */
    
    // BoredOS Treiber-Init aufrufen
    if (bored_nic_init() == 0) {
        // Erfolgreich initialisiert
    } else {
        // Fehlerbehandlung
    }
}