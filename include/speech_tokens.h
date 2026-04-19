/*
 * speech_tokens.h
 * Token definitions and number-to-word lookup tables for spoken time output.
 * Tokens are emitted over UART for the Python TTS bridge to process.
 *
 * Format:  "TOKEN <word>\r\n"
 * End:     "END\r\n"
 */

#ifndef SPEECH_TOKENS_H
#define SPEECH_TOKENS_H

/* ---------------------------------------------------------------------------
 * Fixed tokens
 * --------------------------------------------------------------------------- */
#define TOKEN_THE       "THE"
#define TOKEN_TIME      "TIME"
#define TOKEN_IS        "IS"
#define TOKEN_HOURS     "HOURS"
#define TOKEN_HOUR      "HOUR"
#define TOKEN_MINUTES   "MINUTES"
#define TOKEN_MINUTE    "MINUTE"
#define TOKEN_AND       "AND"
#define TOKEN_SECONDS   "SECONDS"
#define TOKEN_SECOND    "SECOND"

/* ---------------------------------------------------------------------------
 * Number-to-word lookup (0 - 59)
 * Used for hours (0-23) and minutes/seconds (0-59)
 * --------------------------------------------------------------------------- */
static const char * const number_words[] = {
    "ZERO",       "ONE",        "TWO",        "THREE",      "FOUR",
    "FIVE",       "SIX",        "SEVEN",      "EIGHT",      "NINE",
    "TEN",        "ELEVEN",     "TWELVE",     "THIRTEEN",   "FOURTEEN",
    "FIFTEEN",    "SIXTEEN",    "SEVENTEEN",  "EIGHTEEN",   "NINETEEN",
    "TWENTY",     "TWENTY_ONE", "TWENTY_TWO", "TWENTY_THREE","TWENTY_FOUR",
    "TWENTY_FIVE","TWENTY_SIX", "TWENTY_SEVEN","TWENTY_EIGHT","TWENTY_NINE",
    "THIRTY",     "THIRTY_ONE", "THIRTY_TWO", "THIRTY_THREE","THIRTY_FOUR",
    "THIRTY_FIVE","THIRTY_SIX", "THIRTY_SEVEN","THIRTY_EIGHT","THIRTY_NINE",
    "FORTY",      "FORTY_ONE",  "FORTY_TWO",  "FORTY_THREE","FORTY_FOUR",
    "FORTY_FIVE", "FORTY_SIX",  "FORTY_SEVEN","FORTY_EIGHT","FORTY_NINE",
    "FIFTY",      "FIFTY_ONE",  "FIFTY_TWO",  "FIFTY_THREE","FIFTY_FOUR",
    "FIFTY_FIVE", "FIFTY_SIX",  "FIFTY_SEVEN","FIFTY_EIGHT","FIFTY_NINE"
};

/* ---------------------------------------------------------------------------
 * Function declarations
 * --------------------------------------------------------------------------- */

/*
 * Send a single token over UART in the format: "TOKEN <word>\r\n"
 */
void uart_send_token(const char *token);

/*
 * Announce the time by emitting a full token sequence:
 *   TOKEN THE
 *   TOKEN TIME
 *   TOKEN IS
 *   TOKEN <hour_word>
 *   TOKEN HOURS
 *   TOKEN <minute_word>
 *   TOKEN MINUTES
 *   TOKEN AND
 *   TOKEN <second_word>
 *   TOKEN SECONDS
 *   END
 */
void speak_time(uint8_t hour, uint8_t minute, uint8_t second);

#endif /* SPEECH_TOKENS_H */
