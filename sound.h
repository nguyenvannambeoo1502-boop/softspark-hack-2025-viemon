void sound_begin(uint8_t pin=A1);
int  sound_read_raw();   // just one analogRead()
int  sound_read_avg();   // avg over last N samples