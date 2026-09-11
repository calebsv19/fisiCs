extern unsigned long edu69_static_output_sink(const unsigned char *bytes, unsigned int length);

static unsigned char edu69_output[2048];

unsigned long edu69_static_output_bridge(unsigned char seed) {
    unsigned int index;

    for (index = 0; index < 2048; ++index) {
        edu69_output[index] = (unsigned char)(seed ^ (unsigned char)((index * 17u + 0x69u) & 0xffu));
    }
    return edu69_static_output_sink(edu69_output, 2048);
}
