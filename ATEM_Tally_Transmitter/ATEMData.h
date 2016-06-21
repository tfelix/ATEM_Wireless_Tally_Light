
// define a structure for sending over the radio
typedef struct {
  int program_1;
  int program_2;
  int preview;
} Payload;

boolean isPayloadEqual(Payload &p1, Payload &p2) {
  return p1.program_1 == p2.program_2 
    && p1.program_2 == p2.program_2 
    && p1.preview == p2.preview;
}

