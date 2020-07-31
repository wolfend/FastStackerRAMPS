/* Regular (every Arduino loop) display processing stuff
 */

void display_stuff()
{
  // Disabling the last comment line displaying after COMMENT_DELAY interval:
  if (g.comment_flag == 1 && g.t > g.t_comment + COMMENT_DELAY)
  {
    g.comment_flag = 0;
    if (g.moving == 0)
      if (g.alt_flag)
        display_all();
      else
        display_current_position();
        lcd.sendBuffer(); //move frame buffer to display
  }

  // Refreshing battery status regularly (only when not moving, as it is slow):
  if (g.moving == 0 && g.calibrate_warning == 0 && ((g.t - g.t_display) > DISPLAY_REFRESH_TIME))
  {
    g.t_display = g.t;
    battery_status();
    //move just row of tiles from along bottom of frame buffer to display (because doing sendBuffer here caused dicontinuous
    //parameter display that shows when # pressed to go away almost immediately. This essentially, just
    //moves the status line (or at least the slice with the battery symbol) to the display
    lcd.updateDisplayArea(0,7,16,1); 
   }

  return;
}
