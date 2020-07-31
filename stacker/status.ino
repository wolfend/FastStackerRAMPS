/*
 All LCD related functions
*/

void display_all()
/*
 Refreshing the whole screen
 */
{
#ifdef TIMING
  return;
#endif

  if (g.alt_flag)
  {
    lcd.clearBuffer();
    // Row 0:
    sprintf(g.buffer, "Reverse=%1d   Accel=%1d  ", 1 - g.straight, ACCEL_FACTOR[g.i_accel_factor]);
    lcd.drawStr(0,0,g.buffer);
    // Row 1:
    sprintf(g.buffer, "Ntl=%-3d     BckLsh=%1d", N_TIMELAPSE[g.i_n_timelapse], g.backlash_on);
    lcd.drawStr(0,ROW_Y,g.buffer);
    // Row 2:
    sprintf(g.buf6, "TLdt=%ds       ", DT_TIMELAPSE[g.i_dt_timelapse]);
    lcd.drawStr(0,2*ROW_Y,g.buf6);
    sprintf(g.buffer, "MirLck=%1d", g.mirror_lock);
    lcd.drawStr(12*COL_X,2*ROW_Y,g.buffer);
    // Row 3:
    sprintf(g.buffer, "PwrSave=%1d   DisLim=%1d ", g.save_energy, g.disable_limiters);
    lcd.drawStr(0,3*ROW_Y,g.buffer);
    // Row 4 (blank):
    lcd.drawStr(0,4*ROW_Y,"                     ");
    // Row 5:
    sprintf(g.buffer, "           s%s    ", VERSION);
    lcd.drawStr(0,5*ROW_Y,g.buffer);    
  }
  else
  {

    if (g.error == 0)
    {
      if (g.calibrate_warning == 0)
      {
        // display subset of items on main display screen.
        display_u_per_f();  display_fps();
        display_one_point_params();
        display_two_point_params();
        display_two_points();
        display_current_position();
        display_status_line();
      }
      else
      {
        beep();
        lcd.clearBuffer();
        lcd.drawStr(0,0,      "     Calibration     ");
        lcd.drawStr(0,ROW_Y,  "      required!      ");
        lcd.drawStr(0,3*ROW_Y,"   Press any key to  ");
        lcd.drawStr(0,4*ROW_Y,"  start calibration. ");
      }
    }

    else
      // Error code displaying:
    {
      switch (g.error)
      {
        case 1:
          beep();
          lcd.clearBuffer();
          lcd.drawStr(0,0,      "Cable disconnected or");
          lcd.drawStr(0,ROW_Y,  "   limiter is on!    ");
          lcd.drawStr(0,3*ROW_Y," Rewind ONLY if cable");
          lcd.drawStr(0,4*ROW_Y,"   is connected.     ");
          break;

        case 2: // Critically low battery level(not used when debugging)
          beep();
          lcd.clearBuffer();
          lcd.drawStr(0,0,      "   Critically low    ");
          lcd.drawStr(0,ROW_Y,  "   battery level!    ");
          lcd.drawStr(0,3*ROW_Y,"   Replace the       ");
          lcd.drawStr(0,4*ROW_Y,"    batteries.       ");
          break;

      } // case
    }
  }  // if alt_flag
  lcd.sendBuffer(); //move frame buffer to display. 
  return;
  lcd.sendBuffer(); //move frame buffer to display
}
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

void letter_status(char* l)
/*
 Display a letter code "l" at the beginning of the status line
 */
{
  if (g.error || g.alt_flag)
    return;
  if (g.paused)
    lcd.drawStr(0,5*ROW_Y,"P");
  else if (g.timelapse_mode)
    lcd.drawStr(0,5*ROW_Y,"T");
  else
    lcd.drawStr(0,5*ROW_Y,l);
  return;
}
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

void motion_status()
/*
 Motion status: 3-char status showing the direction and speed (rewind vs. focus stacking)
 */
{
  if (g.error || g.alt_flag)
    return;
  uint8_t i;
    if (g.moving == 0 && g.started_moving == 0)
    lcd.drawStr(COL_X,5*ROW_Y,"   ");
  else
  {
    if (g.direction == -1)
    {
      if (g.stacker_mode < 2)
        
          lcd.drawStr(COL_X,5*ROW_Y,"<<-");
          
      else
        lcd.drawStr(COL_X,5*ROW_Y," < ");
    }
    else
    {
      if (g.stacker_mode < 2)
        lcd.drawStr(COL_X,5*ROW_Y, "->>");
      else
        lcd.drawStr(COL_X,5*ROW_Y," > ");
    }
  }
  lcd.sendBuffer(); //move frame buffer to display
  return;
}
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++


void display_frame_counter()
/*
 Printing the current stacking frame number in the status line.
  */
{
  if (g.error || g.alt_flag)
    return;
  // Printing frame counter:
  if (g.stacker_mode == 0 && g.paused == 0 || g.paused > 1){
    sprintf (g.buffer, "   0   ");
  }
  else
    {
      sprintf (g.buffer, "  %4d ",  g.frame_counter + 1);
    }
    lcd.drawStr(4*COL_X,5*ROW_Y,g.buffer);
  lcd.sendBuffer();
  return;
}
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++


void points_status()
/* Displays Fore or Back if the current coordinate is exactly the foreground (g.point1) or background (g.point2) point.
 */
{
  if (g.error || g.alt_flag)
    return;
  if (g.pos_short_old == g.point1)
    lcd.drawStr(11*COL_X,5*ROW_Y,"   Fore");
  else if (g.pos_short_old == g.point2)
    lcd.drawStr(11*COL_X,5*ROW_Y,"   Back");
  else
    lcd.drawStr(11*COL_X,5*ROW_Y,"           ");

  return;
}
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
void battery_status()
/*
 Measuring the battery voltage and displaying it.
 */
{
  if (g.moving == 1 || g.alt_flag)
    return;

  // Battery voltage measured via a two-resistor voltage divider
  // Slow operation (100 us), so should be done infrequently
  float V = (float)analogRead(PIN_BATTERY) * VOLTAGE_SCALER;

  // This is done only once, when the decice is powered up:
  if (g.setup_flag == 1)
  {
    // Deciding which speed limit to use: using the larger value if powered from AC, smaller value if powered from batteries:
    if (V > SPEED_VOLTAGE)
      g.speed_limit = SPEED_LIMIT;
    else
      g.speed_limit = SPEED_LIMIT2;
  }

  if (g.error)
    return;

#ifdef BATTERY_DEBUG
  // Printing actual voltage per AA battery (times 100)
  int Vint = (int)(100.0 * V);
  sprintf(g.buffer, "%3d", Vint);
  lcd.drawStr(0*COL_X,5*ROW_Y+2,g.buffer);
#else
  // A 4-level indication (between V_LOW and V_HIGH):
  byte level = (short)((V - V_LOW) / (V_HIGH - V_LOW) * 4.0);
  if (level < 0)
    level = 0;
  if (level > 3)
    level = 3;
  displayBatteryStatus(111,57,level);

#endif // BATTERY_DEBUG

  // Disabling the rail once V goes below the critical V_LOW voltage
#ifndef MOTOR_DEBUG
  if (V < V_LOW)
    float V = (float)analogRead(PIN_BATTERY) * VOLTAGE_SCALER; //read again to make sure before shutting down
    if (V < V_CRITICAL)
      g.error = 2;
#endif

  return;
}
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

void display_status_line()
/*
 Display the whole status line
 */
{
  if (g.error || g.alt_flag)
    return;
  letter_status(" ");
  motion_status();
  display_frame_counter();
  points_status();
  battery_status();
  lcd.sendBuffer(); //move frame buffer to display
  return;
}
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++


void display_u_per_f()
/*
 Display the input parameter u per frame (1000*MM_PER_FRAME)
 */
{
  if (g.error || g.alt_flag)
    return;

  if (MM_PER_FRAME[g.i_mm_per_frame] >= 0.00995)
    sprintf(g.buffer, "% 4du/f   ", nintMy(1000.0 * MM_PER_FRAME[g.i_mm_per_frame]));
  else
    // +0.05 is for proper round-off:
    sprintf(g.buffer, "%4su/f   ", ftoa(g.buf7, 1000.0 * MM_PER_FRAME[g.i_mm_per_frame] + 0.05, 1));

  lcd.drawStr(0,2*ROW_Y,g.buffer);
  return;
}
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++


void display_fps()
/*
 Display the input parameter fps (frames per second)
 */
{
  if (g.error || g.alt_flag)
    return;
  if (FPS[g.i_fps] >= 1.0)
    sprintf(g.buffer, " %3sfps  ", ftoa(g.buf7, FPS[g.i_fps], 1));
  else
    sprintf(g.buffer, "%4sfps  ", ftoa(g.buf7, FPS[g.i_fps], 2));

  lcd.drawStr(11*COL_X,2*ROW_Y,g.buffer);
  return;
}
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++


void display_one_point_params()
/*
 Display the three parameters for one-point shooting:
  - input parameter N_SHOTS,
  - travel distance, mm,
  - travel time, s.
 */
{
  if (g.error || g.alt_flag)
    return;

  // +0.05 for proper round off:
  float dx = (float)(N_SHOTS[g.i_n_shots] - 1) * MM_PER_FRAME[g.i_mm_per_frame] + 0.05;
  short dt = (short)roundMy((float)(N_SHOTS[g.i_n_shots] - 1) / FPS[g.i_fps]);
  if (dt < 1000.0 && dt >= 0.0)
    sprintf(g.buf6, "%3ds", dt);
  else if (dt < 10000.0 && dt >= 0.0)
    sprintf(g.buf6, "%4d", dt);
  else
    sprintf(g.buf6, "****");

  if (dx < RAIL_LENGTH)
    ftoa(g.buf7, dx, 1);
  else
    sprintf(g.buf7, "****");

  sprintf(g.buffer, "1p:%4df %4smm %4s", N_SHOTS[g.i_n_shots], g.buf7 , g.buf6);
  lcd.drawStr(0,0,g.buffer);
  return;
}
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++


void display_two_point_params()
/*
 Display the three parameters for two-point shooting:
  - number of shots,
  - travel distance, mm,
  - travel time, s.
 */
{
  if (g.error || g.alt_flag)
    return;

  // +0.05 for proper round off
  float dx = MM_PER_MICROSTEP * (float)(g.point2 - g.point1) + 0.05;
  short dt = (short)nintMy((float)(g.Nframes - 1) / FPS[g.i_fps]);
  if (dt < 1000.0 && dt >= 0.0)
    sprintf(g.buf6, "%3ds", dt);
  else if (dt < 10000.0 && dt >= 0.0)
    sprintf(g.buf6, "%4d", dt);
  else
    sprintf(g.buf6, "****");

  if (g.point2 >= g.point1)
    sprintf(g.buffer, "2p:%4df %4smm %4s ", g.Nframes, ftoa(g.buf7, dx, 1), g.buf6);
  else
    sprintf(g.buffer, "2p:  **** ***** *****");
  lcd.drawStr(0,ROW_Y,g.buffer);
  return;
}
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++


void display_two_points()
/*
 Display the positions (in mm) of two points: foreground, F, and background, B.
 */
{
  if (g.error || g.alt_flag)
    return;

  float p = MM_PER_MICROSTEP * (float)g.point1;
  if (p >= 0.0)
    sprintf(g.buffer, "F:%s   ", ftoa(g.buf7, p, 3));
  else
    sprintf(g.buffer, "F:*****");
  lcd.drawStr(0,3*ROW_Y,g.buffer);
  p = MM_PER_MICROSTEP * (float)g.point2;
  if (p >= 0.0)
    sprintf(g.buffer, "B:%s   ", ftoa(g.buf7, p, 3));
  else
    sprintf(g.buffer, "B:*****");
    lcd.drawStr(10*COL_X,3*ROW_Y,g.buffer);
  return;
}
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++


void display_current_position()
/*
 Display the current position on the transient line
 */
{
#ifdef CAMERA_DEBUG
  //  return;
#endif
#ifdef TIMING
  // Average loop length for the last motion, in shortest miscrostep length units *100:
  short avr = (short)(100.0 * (float)(g.t - g.t0_timing) / (float)(g.i_timing - 1) * SPEED_LIMIT);
  // Maximum/minimum loop lenght in the above units:
  short max1 = (short)(100.0 * (float)(g.dt_max) * SPEED_LIMIT);
  short min1 = (short)(100.0 * (float)(g.dt_min) * SPEED_LIMIT);
  sprintf(g.buffer, "%4d %4d %4d", min1, avr, max1);
  lcd.drawStr(0,4*ROW_Y,g.buffer);
  // How many times arduino loop was longer than the shortest microstep time interval; total number of arduino loops:
  sprintf(g.buffer, "%4d %6ld   ", g.bad_timing_counter, g.i_timing);
  lcd.drawStr(0,5*ROW_Y,g.buffer);
#ifdef MOTOR_DEBUG
  sprintf(g.buffer, "%4d %4d %4d", cplus2, cmax, imax);
  lcd.drawStr(0,3*ROW_Y,g.buffer);
#endif
  return;
#endif

  if (g.error || g.calibrate_warning || g.moving == 0 && g.BL_counter > (COORD_TYPE)0 || g.alt_flag)
    return;

  if (g.straight)
    g.rev_char = " ";
  else
    g.rev_char = "R";

  if (g.timelapse_mode)
    sprintf(g.buf6, "%3d", g.timelapse_counter + 1);
  else
    sprintf(g.buf6, "    ");

#ifdef BL_DEBUG
// When debugging backlash, displays the current backlash value in microsteps
  sprintf(g.buf6, "%3d", g.backlash);
#endif
#ifdef BL2_DEBUG
// When debugging BACKLASH_2, displays the current BACKLAS_2 value in microsteps
  sprintf(g.buf6, "%3d", BACKLASH_2);
#endif
#ifdef DELAY_DEBUG
// Delay used in mirror_lock=2 mode (electronic shutter), in 10ms units:
  sprintf(g.buf6, "%3d", SHUTTER_ON_DELAY2/10000);
#endif

  float p = MM_PER_MICROSTEP * (float)g.pos;
  sprintf(g.buffer, "%1sCurPos:%7smm %3s", g.rev_char, ftoa(g.buf7, p, 3), g.buf6); //display: the "R" indicator, the current position, time lapse stack #

  lcd.drawStr(0,4*ROW_Y,g.buffer);

  return;
}
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++


void display_comment_line(char* l)
/*
 Display a comment line briefly (then it should be replaced with display_current_positio() output)
 */
{
#ifdef TIMING
  return;
#endif
#ifdef CAMERA_DEBUG
  return;
#endif
  if (g.error)
    return;
  lcd.drawStr(0,4*ROW_Y,l);
  lcd.sendBuffer(); //move frame buffer to display
  g.t_comment = g.t;
  g.comment_flag = 1;
  return;
}

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

/* Display a battery state of charge display*/
void displayBatteryStatus(uint8_t x, uint8_t y, uint8_t value)
{
  //draw the battery
  lcd.drawFrame(x,y,15,7);
  lcd.drawVLine(x+15,y+2,3);
  lcd.drawVLine(x+16,y+2,3);

  //now the state of charge
  lcd.setDrawColor(0);
  lcd.drawBox(x+1,y+1,13,5); //clear the old SOC
  lcd.setDrawColor(1);
  switch (value) {
    case 0:
      break;
    case 1:
      lcd.drawBox(x+1,y+1,4,5);
      break;
    case 2:
      lcd.drawBox(x+1,y+1,8,5);
      break;
  case 3:
      lcd.drawBox(x+1,y+1,14,5);
      break;
  }
  return;
}

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

void delay_buffer()
// Fill g.buffer with non-continuous stacking parameters, to be displayed with display_comment_line:
{
  float y = MM_PER_FRAME[g.i_mm_per_frame] / MM_PER_MICROSTEP / ACCEL_LIMIT;
  // Time to travel one frame (s), with fixed acceleration:
  float dt_goto = 2e-6 * sqrt(y);
  float delay1 = FIRST_DELAY[g.i_first_delay];
  float delay2 = SECOND_DELAY[g.i_second_delay];
  short dt = (short)nintMy((float)(g.Nframes) * (FIRST_DELAY[g.i_first_delay] + SECOND_DELAY[g.i_second_delay]) + (float)(g.Nframes - 1) * dt_goto);
  sprintf(g.buffer, "T1:%4s T2:%4s %4ds", ftoa(g.buf7, delay1, 1), ftoa(g.buf6, delay2, 1), dt);

  return;
}
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++


void beep()
// short beep. It delays, so use only when halting for error message
 {
  digitalWrite(BEEPER, HIGH);
  delay(100);
  digitalWrite(BEEPER, LOW);
  return;
}
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++


#ifdef CAMERA_DEBUG
void AF_status(short s)
{
  lcd.drawStr(12*COL_X,4*ROW_Y,s);
  return;
}
void shutter_status(short s)
{
  lcd.drawStr(13*COL_X,4*ROW_Y,s);
  return;
}
#endif
