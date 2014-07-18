#include "vt100sim.h"
#include <cdk/cdk.h>

int main(int argc, char *argv[])
{
  CDKSCREEN   *cdkscreen;
  CDKLABEL    *demo;
  WINDOW      *screen;
  char        *mesg[4];
  
  /* Initialize the Cdk screen.   */
  screen = initscr();
  cdkscreen = initCDKScreen (screen);
  
  /* Start CDK Colors */
  initCDKColor();
  
  /* Set the labels up.      */
  mesg[0] = "</31>This line should have a yellow foreground and a cyan background.<!31>";
  mesg[1] = "</05>This line should have a white  foreground and a blue background.<!05>";
  mesg[2] = "</26>This line should have a yellow foreground and a red  background.<!26>";
  mesg[3] = "<C>This line should be set to whatever the screen default is.";
  
  /* Declare the labels.     */
  demo   = newCDKLabel (cdkscreen, CENTER, CENTER, mesg, 4, TRUE, TRUE);
  
  /* Draw the label          */
  drawCDKLabel (demo, TRUE);

  sim = new Vt100Sim(argv[1]);
  sim->init();
  for (int i = 0; i < 10000; i++) { sim->step(); }

  waitCDKLabel (demo, ' ');
  
  /* Clean up           */
  destroyCDKLabel (demo);
  destroyCDKScreen (cdkscreen);
  endCDK();
  
}
