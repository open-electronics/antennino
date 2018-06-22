
#define LED 9 


// the setup function runs once when you press reset or power the board
void setup() {
  // initialize digital pin LED_BUILTIN as an output.
  pinMode(LED, OUTPUT);
}



// the loop function runs over and over again forever
void loop() {

  // turn the LED on (HIGH is the voltage level)
  digitalWrite(LED, HIGH);   
  
  delay(1000);   
  // wait for a second
  
  // turn the LED off by making the voltage LOW
  digitalWrite(LED, LOW);    
  
  // wait for a second
  delay(1000);  
  
}
