typedef unsigned wave15_word_t;

static wave15_word_t wave15_words[3] = {3u, 5u, 8u};

wave15_word_t wave15_multitu_object_alias_bridge(void) {
    return wave15_words[0] + wave15_words[1] + wave15_words[2];
}
