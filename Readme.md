# Arduino dice simulator

This is a program for Arduino Uno working as a DnD dice.

## Instalation

Just upload this to an Arduino Uno via Arduino IDE and everything should work.

## Assignment and usage

The objective is to create a random number generator which will simulate dice throws in Advanced Dungeons & Dragons game. The game uses various types of polyhedral dices with different numbers of sides (d4, d6, d8, ...). Furthermore, a generated random number represents a sum of multiple throws. E.g., 3d6 means that player should throw 3 times using a dice with 6 sides (cube) and take the sum of these three throws (which is a number between 3 and 18).

The dice is controlled by 3 buttons and display the results on the 4-digit LED display. It operates in two modes. In normal mode it displays last generated number on the LED display. In configuration mode it displays the type (and repetition) of dice being simulated. First digit (1-9) displays number of throws, second digit displays symbol 'd', and the remaining two digits display the type of dice (d4, d6, d8, d10, d12, d20, and d100 should be supported; d100 is denoted as '00' on the LED display).

-Button 1
  switches the dice to normal mode
  whilst pressed down, the random data are gathered (result is being generated)
  when the button is released, a new random result has to be displayed
-Button 2
  switches the dice to configuration mode
  increments the number of throws (if 9 is exceeded, the number returns to 1)
-Button 3
  switches the dice to configuration mode
  changes the dice type (dices d4-d100 from the list above are cycled)

It might be a good idea to show some 'activity' on the display whilst the random number is being generated (whilst the button 1) is pressed. You may show currently computed random numbers (if they change fast enought so the user cannot possibly stop at the right number), or you may create some sort of animation on LED display or using the other onboard LEDs.

Remember that the probability distribution is not uniform, but if follows Binomial distribution. Your simulator must use counter/time measurement of how long the button 1 has been pressed to get a random number (which follows uniform distribution). You may use additional pseudo-random generators (even Arduino built-in functions) to assist you with this task, but the initial randomness has to be tied somehow to the button event duration. Any reasonable implementation of binomial distribution will be accepted, but strictly uniform random generators will not (i.e., you need to implement binomial distribution using uniform random generator).
