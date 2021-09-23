#include "funshield.h"

// map of letter glyphs
constexpr byte LETTER_GLYPH[]{
    0b10001000, // A
    0b10000011, // b
    0b11000110, // C
    0b10100001, // d
    0b10000110, // E
    0b10001110, // F
    0b10000010, // G
    0b10001001, // H
    0b11111001, // I
    0b11100001, // J
    0b10000101, // K
    0b11000111, // L
    0b11001000, // M
    0b10101011, // n
    0b10100011, // o
    0b10001100, // P
    0b10011000, // q
    0b10101111, // r
    0b10010010, // S
    0b10000111, // t
    0b11000001, // U
    0b11100011, // v
    0b10000001, // W
    0b10110110, // ksi
    0b10010001, // Y
    0b10100100, // Z
};
constexpr byte EMPTY_GLYPH = 0b11111111;

constexpr size_t ASCII_CONSTANT = 48;
constexpr size_t SIZE_LIMIT = 1024;
constexpr size_t DIE_SIZES[] = {4, 6, 8, 10, 12, 20, 100};
constexpr size_t N_DIE_SIZES = 7;
constexpr size_t EMPTY_PREFIX = 4;
constexpr unsigned int SCROLLING_INTERVAL = 300;
constexpr size_t MAX_DICE = 9;
constexpr size_t N_DIGITS = 4;
constexpr int DEBOUNCE_TIME = 20;
constexpr int CONTINUOUS_TIME = 400;
constexpr size_t BLACK = 10; // Which index coresponds to black display glyph
char IntToChar(size_t num)   // Get last digit of number as char
{
    return (char)(num % 10) + ASCII_CONSTANT;
}
enum class DnDState
{
    NORMAL,
    CONFIG,
    ROLLING
};
enum class ButtonState
{
    DEBOUNCE,
    FREE,
    PRESSED
};

struct Button
{
    long unsigned int next_time;
    int pin;
    ButtonState state;
    void setup(int pin_value)
    {
        state = ButtonState::FREE;
        pin = pin_value;
        pinMode(pin, INPUT);
        next_time = 0;
    }
    bool Check(long unsigned int current_time)
    {
        if (!digitalRead(pin))
        {
            return Pressed(current_time);
        }
        else
        {
            return Unpressed(current_time);
        }
    }
    bool Pressed(long unsigned int current_time)
    {
        if (current_time > next_time)
        {
            next_time = current_time + CONTINUOUS_TIME;
            state = ButtonState::PRESSED;
            return true;
        }
        return false;
    }
    bool Unpressed(long unsigned int current_time)
    {
        if (state == ButtonState::PRESSED)
        {
            next_time = current_time + DEBOUNCE_TIME;
            state = ButtonState::DEBOUNCE;
        }
        else if (state == ButtonState::DEBOUNCE && current_time > next_time)
        {
            state = ButtonState::FREE;
        }
        return false;
    }
};

struct ButtonC
{
    // This could be derived from Button class, but this one is so simple, this is more concise...
    long unsigned int next_time;
    int pin;
    void setup(int pin_value)
    {
        pin = pin_value;
        pinMode(pin, INPUT);
        next_time = 0;
    }
    bool Check(long unsigned int current_time)
    {
        if (!digitalRead(pin))
        {
            next_time = current_time + DEBOUNCE_TIME;
            return true;
        }
        else
        {
            if (current_time > next_time) // Hold true for debounce time
            {
                return false;
            }
            return true;
        }
    }
};

struct Display
{
    byte showing[N_DIGITS];
    size_t show_next = 0;

    void setup()
    {
        pinMode(latch_pin, OUTPUT);
        pinMode(clock_pin, OUTPUT);
        pinMode(data_pin, OUTPUT);
    }

    void ShowByte(size_t segment, byte value)
    {
        // Send signal for 1 control byte
        digitalWrite(latch_pin, LOW);
        shiftOut(data_pin, clock_pin, MSBFIRST, value);
        shiftOut(data_pin, clock_pin, MSBFIRST, digit_muxpos[segment]);
        digitalWrite(latch_pin, HIGH);
    }

    void ShowChar(size_t segment, char ch)
    {
        //Translate character to control byte
        byte glyph = EMPTY_GLYPH;
        if (isAlpha(ch))
            glyph = LETTER_GLYPH[ch - (isUpperCase(ch) ? 'A' : 'a')];
        else if (isDigit(ch))
            glyph = digits[ch - '0'];
        showing[segment] = glyph;
    }

    void ShowString(char characters[])
    {
        //Show 4 character string.
        for (size_t i = 0; i < N_DIGITS; ++i)
        {
            ShowChar(i, characters[i]);
        }
    }

    void Update()
    {
        // Call this every cycle
        ShowByte(show_next, showing[show_next]);
        show_next = (show_next + 1) % N_DIGITS;
    }
};
struct Die
{
    size_t sides;
    unsigned int inner_bias;
    size_t result;

    Die()
    {
        sides = 0;
        inner_bias = (random(0, DIE_SIZES[N_DIE_SIZES - 1] * MAX_DICE) * random(0, DIE_SIZES[N_DIE_SIZES - 1] * MAX_DICE));
    }

    size_t Throw(long unsigned int current_time, size_t bias)
    {
        if (sides == 0)
            return 0;
        return ((current_time + inner_bias + bias +
                 random(0, DIE_SIZES[N_DIE_SIZES - 1])) %
                sides) +
               1;
    }
    void ChangeSides(size_t n)
    {
        sides = n;
    }
};
struct DnD
{
    Display *d;
    Die *dice;
    DnDState state;
    size_t bias;
    size_t result;
    size_t active_dice;
    size_t die_size_index;
    char displayable[N_DIGITS] = {'s', 'e', 't', 't'};

    void StartRoll(long unsigned int current_time)
    {
        state = DnDState::ROLLING;
        bias = current_time % (DIE_SIZES[N_DIE_SIZES - 1] * MAX_DICE);
    }
    void Roll(long unsigned int current_time)
    {
        result = 0;
        for (size_t i = 0; i < MAX_DICE; ++i)
        {
            result += dice[i].Throw(current_time, bias);
        }
    }
    void AddDie()
    {
        active_dice = (active_dice % MAX_DICE) + 1;
        if (active_dice == 1)
        {
            for (size_t i = 1; i < MAX_DICE; ++i)
            {
                dice[i].ChangeSides(0);
            }
        }
        else
            dice[active_dice - 1].ChangeSides(DIE_SIZES[die_size_index]);
    }
    void NextDieType()
    {
        die_size_index = (die_size_index + 1) % N_DIE_SIZES;
        for (size_t i = 0; i < active_dice; ++i)
        {
            dice[i].ChangeSides(DIE_SIZES[die_size_index]);
        }
    }
    void ShowResult()
    {
        // Show results of given roll
        size_t temp = result;
        for (size_t i = 0; i < N_DIGITS; ++i)
        {
            if (temp > 0)
            {
                displayable[N_DIGITS - i - 1] = IntToChar(temp % 10);
            }
            else
            {
                displayable[N_DIGITS - i - 1] = ' ';
            }
            temp /= 10;
        }
    }
    void ShowConfig()
    {
        // Show config screen (1d6)
        displayable[0] = IntToChar(active_dice);
        displayable[1] = 'd';
        if ((DIE_SIZES[die_size_index]) < 10)
        {
            displayable[2] = ' ';
        }
        else
        {
            displayable[2] = IntToChar(DIE_SIZES[die_size_index] / 10);
        }
        displayable[3] = IntToChar(DIE_SIZES[die_size_index]);
    }
    void ShowRolling()
    {
        // Show something while the dies are rolling
        // I know it is not an animation, hope this is enough ;-)
        displayable[0] = 'r';
        displayable[1] = 'o';
        displayable[2] = 'l';
        displayable[3] = 'l';
    }

    void UpdateDisplay()
    {
        // Call this if what should be displayed has changed.
        ShowRolling();
        if (state == DnDState::NORMAL)
            ShowResult();
        else if (state == DnDState::CONFIG)
            ShowConfig();
        else
            ShowRolling();
        d->ShowString(displayable);
    }
    void setup(Display *dis)
    {
        die_size_index = N_DIE_SIZES - 1;
        active_dice = 0;
        result = 0;
        d = dis;
        state = DnDState::CONFIG;
        dice = new Die[MAX_DICE];
        AddDie();
        NextDieType();
        UpdateDisplay();
    };
    void Button1P(long unsigned int current_time) //Button 1 is pressed
    {
        // Button 1 is pressed
        if (state != DnDState::ROLLING)
        {
            StartRoll(current_time);
            UpdateDisplay();
        }
    }

    void Button1U(long unsigned int current_time) //Button 1 is not pressed
    {
        // Button 1 is NOT pressed
        if (state == DnDState::ROLLING)
        {
            state = DnDState::NORMAL;
            Roll(current_time);
            UpdateDisplay();
        }
    }

    void Button2()
    {
        if (state == DnDState::NORMAL)
        {
            state = DnDState::CONFIG;
            UpdateDisplay();
        }
        else if (state == DnDState::CONFIG)
        {
            AddDie();
            UpdateDisplay();
        }
    }

    void Button3()
    {
        if (state == DnDState::NORMAL)
        {
            state = DnDState::CONFIG;
            UpdateDisplay();
        }
        else if (state == DnDState::CONFIG)
        {
            NextDieType();
            UpdateDisplay();
        }
    }
};
long unsigned int current_time;
Display d;
DnD core;
ButtonC b1;
Button b2;
Button b3;
int i = 0;
void setup()
{
    b1.setup(button1_pin);
    b2.setup(button2_pin);
    b3.setup(button3_pin);
    d.setup();
    core.setup(&d);
}
void loop()
{
    current_time = millis();
    if (b1.Check(current_time))
        core.Button1P(current_time);
    else
        core.Button1U(current_time);
    if (b2.Check(current_time))
        core.Button2();
    if (b3.Check(current_time))
        core.Button3();
    d.Update();
}
