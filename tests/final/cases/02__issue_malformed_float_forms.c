/* Historical repro: malformed float spellings are currently accepted. */
double a = 1e+;
double b = 1e;
double c = 0x1.p;
