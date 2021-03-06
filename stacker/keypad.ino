void process_keypad()
/*
 All the keypad runtime stuff goes here
 */
{
  float speed;
  short frame_counter0;
  COORD_TYPE pos_target;


  // Ignore keypad during emergency breaking
  //  if (g.breaking == 1 || (g.calibrate == 3 && g.calibrate_warning == 0) || g.error > 1)
  if (g.breaking == 1 || g.error > 1)
    return;

  // This is to keep the non-continuous parameters displayed as long as the key "#" is pressed:
  if (g.comment_flag == 1 && keypad.key[0].kchar == '#')
    // -COMMENT_LINE+10000 is a hack, to reduce to almost zero the time the parameters are displayed:
    // (So basically the parameters are only displayed as long as the "#" key is pressed)
    g.t_comment = g.t - COMMENT_DELAY + 10000;

  // The previous value of the key 0:
  g.key_old = keypad.key[0].kchar;

  // This is a trick to generate multiple actions when you press certain keys (like "2") long enough
  // (longer than T_KEY_LAG). The multiple actions are separated by delays T_KEY_REPEAT.
  // The trick is to generate fake key press events for the currently pressed key. Flag fake_key
  // is used to differentiate bwetween a real key press (then it is '0') and fake key press (it is '1').
  char fake_key = 0;
  // This is the list of the all keys (only one-key bindings are allowed) with multiple actions:
  if ((g.key_old == '2' || g.key_old == '3' || g.key_old == '5' || g.key_old == '6' || g.key_old == '8' || g.key_old == '9')
      && g.t - g.t_key_pressed > T_KEY_LAG)
    // We are here when a change parameter key was pressed longer than T_KEY_LAG
  {
    if (g.N_repeats == 0)
      // Generating the first fake key event:
    {
      g.t_last_repeat = g.t;
      g.N_repeats = 1;
      fake_key = 1;
    }
    if (g.t - g.t_last_repeat > T_KEY_REPEAT)
      // Generating subsequent fake key events:
    {
      g.N_repeats++;
      g.t_last_repeat = g.t;
      fake_key = 1;
    }
  }


  // Rescanning the keys. Most of the calls return false (no scan performed), exiting immediately if so
  if (!keypad.getKeys() && !fake_key)
    return;

  KeyState state0, state1;
  char key0;
  bool state0_changed;
  if (fake_key)
    // Simulating a fake key (for repetitive key actions when certain keys are pressed long enough)
  {
    state0 = PRESSED;
    key0 = g.key_old;
    state0_changed = 1;
  }
  else
    // Processing real (not fake) key press
  {
    state0 = keypad.key[0].kstate;
    state1 = keypad.key[1].kstate;
    key0 = keypad.key[0].kchar;
    state0_changed = keypad.key[0].stateChanged;
  }


  //+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
  // Two-key #X commands (no fake key events allowed)
  if (state0 == PRESSED && keypad.key[0].kchar == '#' && keypad.key[1].stateChanged && state1 == PRESSED && !fake_key)
  {
    switch (keypad.key[1].kchar)
    {
      case 'C': // #C: Initiate a full calibration
        // Ignore if moving:
        if (g.moving == 1 || g.paused)
          break;
        g.calibrate = 3;
        g.calibrate_flag = 0;
        g.calibrate_warning = 1;
        g.calibrate_init = g.calibrate;
        // Displaying the calibrate warning:
        display_all();
        break;

      case 'B':  // #B: Initiate emergency braking, or abort paused stacking
        if (g.paused && g.moving == 0)
          // Aborting stacking:
        {
          g.paused = 0;
          g.frame_counter = 0;
          display_frame_counter();
          g.noncont_flag = 0;
          g.stacker_mode = 0;
          g.timelapse_mode = 0;
          g.end_of_stacking = 0;
          display_all();
        }
        else
          // Emergency braking:
        {
          change_speed(0.0, 0, 2);
          // This should be done after change_speed(0.0):
          g.breaking = 1;
          letter_status("B");
          g.calibrate = 0;
          g.calibrate_flag = 0;
          g.calibrate_warning = 0;
          g.calibrate_init = g.calibrate;
        }
        break;

      case '2': // #2: Save parameters to first memory bank
        if (g.paused)
          break;
        save_params(ADDR_REG1, 1);
        break;

      case '3': // #3: Read parameters from first memory bank
        if (g.paused)
          break;
        read_params(ADDR_REG1, 1);
        break;

      case '5': // #5: Save parameters to second memory bank
        if (g.paused)
          break;
        save_params(ADDR_REG2, 2);
        break;

      case '6': // #6: Read parameters from second memory bank
        if (g.paused)
          break;
        read_params(ADDR_REG2, 2);
        break;

      case '8': // #8: Cycle through the table for FIRST_DELAY parameter
        if (g.paused)
          break;
        if (g.i_first_delay < N_FIRST_DELAY - 1)
          g.i_first_delay++;
        else
          g.i_first_delay = 0;
        EEPROM.put( ADDR_I_FIRST_DELAY, g.i_first_delay);
        // Fill g.buffer with non-continuous stacking parameters, to be displayed with display_comment_line:
        delay_buffer();
        display_comment_line(g.buffer);
        break;

      case '9': // #9: Cycle through the table for SECOND_DELAY parameter
        if (g.paused)
          break;
        if (g.i_second_delay < N_SECOND_DELAY - 1)
          g.i_second_delay++;
        else
          g.i_second_delay = 0;
        EEPROM.put( ADDR_I_SECOND_DELAY, g.i_second_delay);
        // Fill g.buffer with non-continuous stacking parameters, to be displayed with display_comment_line:
        delay_buffer();
        display_comment_line(g.buffer);
        break;

      case '4': // #4: not used (was backlight control)
        break;

      case '*': // #*: Factory reset
        if (g.paused)
          break;
        initialize(1);
        break;

      case '7': // #7: Manual camera shutter triggering
        if (g.moving)
          break;
        g.make_shot = 1;
        g.single_shot = 1;
        g.start_stacking = 0;
        g.stacker_mode = 0;
        break;

      case '1': // #1: Rewind a single frame step (no shooting)
        if (g.moving || g.paused > 1)
          break;
        frame_counter0 = g.frame_counter;
        if (g.paused)
        {
          g.frame_counter--;
          pos_target = frame_coordinate();
        }
        else
        {
          g.frame_counter--;
          pos_target = (COORD_TYPE)(g.pos - g.msteps_per_frame);
        }
        // This 100 steps padding is just a hack, to fix the occasional bug when a combination of single frame steps and rewind can
        // move the rail beyond g.limit1
        if (pos_target < g.limit1 + (COORD_TYPE)100 || pos_target > g.limit2 - (COORD_TYPE)100 || g.paused && (g.frame_counter < 0 || g.frame_counter >= g.Nframes))
        {
          // Recovering the original frame counter if aborting:
          g.frame_counter = frame_counter0;
          break;
        }
        go_to(pos_target + 0.5, g.speed_limit);
        display_frame_counter();
        break;

      case 'A': // #A: Fast-forward a single frame step (no shooting)
        if (g.moving || g.paused > 1)
          break;
        // Required microsteps per frame:
        frame_counter0 = g.frame_counter;
        if (g.paused)
        {
          g.frame_counter++;
          pos_target = frame_coordinate();
        }
        else
        {
          g.frame_counter++;
          pos_target = (COORD_TYPE)(g.pos + g.msteps_per_frame);
        }
        if (pos_target < g.limit1 + (COORD_TYPE)100 || pos_target > g.limit2 - (COORD_TYPE)100 || g.paused && (g.frame_counter < 0 || g.frame_counter >= g.Nframes))
        {
          g.frame_counter = frame_counter0;
          break;
        }
        go_to(pos_target + 0.5, g.speed_limit);
        display_frame_counter();
        break;

      case 'D':  // #D: Go to the last starting point (for both 1- and 2-point shooting); not memorized in EEPROM
        if (g.paused)
          break;
        go_to((float)g.starting_point + 0.5, g.speed_limit);
        display_comment_line(" Going to last start ");
        break;

      case '0': // #0: Start 2-point focus stacking from the foreground point in a non-continuous mode
        if (g.paused)
          break;
        // Checking the correctness of point1/2
        if (g.point2 > g.point1 && g.point1 >= g.limit1 && g.point2 <= g.limit2)
        {
          // Using the simplest approach which will result the last shot to always slightly undershoot
          g.Nframes = Nframes();
          // Always starting from the foreground point, for full backlash compensation:
          go_to((float)g.point1 + 0.5, g.speed_limit);
          g.starting_point = g.point1;
          g.destination_point = g.point2;
          g.stacker_mode = 1;
          // This is a non-continuous mode:
          g.continuous_mode = 0;
          g.start_stacking = 0;
          g.timelapse_counter = 0;
          if (N_TIMELAPSE[g.i_n_timelapse] > 1)
            g.timelapse_mode = 1;
          display_comment_line("     2-points stack  ");
        }
        else
        {
          // Should print error message
          display_comment_line("    Bad 2 points!    ");
        }
        break;

    } // switch
  }


  //+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
  // Two-key *X commands (don't work for paused and moving states; no fake key events allowed)
  else if (state0 == PRESSED && keypad.key[0].kchar == '*' && keypad.key[1].stateChanged && state1 == PRESSED && !fake_key)

  {
    if (g.moving == 0 && g.paused == 0)
    {
      switch (keypad.key[1].kchar)
      {
        case '1': // *1: Rail reverse
          g.straight = 1 - g.straight;
          EEPROM.put( ADDR_STRAIGHT, g.straight);
          display_all();
          // Reversing the rail and updating the point1,2 parameters:
          rail_reverse(1);
          break;

        case '2': // *2: Save parameters to third memory bank
          save_params(ADDR_REG3, 3);
          break;

        case '3': // *3: Read parameters from third memory bank
          read_params(ADDR_REG3, 3);
          break;

        case '5': // *5: Save parameters to fourth memory bank
          save_params(ADDR_REG4, 4);
          break;

        case '6': // *6: Read parameters from fourth memory bank
          read_params(ADDR_REG4, 4);
          break;

        case '8': // *8: Save parameters to fifth memory bank
          save_params(ADDR_REG5, 5);
          break;

        case '9': // *9: Read parameters from fifth memory bank
          read_params(ADDR_REG5, 5);
          break;

        case 'A': // *A: Change accel_factor
          if (g.i_accel_factor < N_ACCEL_FACTOR - 1)
            g.i_accel_factor++;
          else
            g.i_accel_factor = 0;
          EEPROM.put( ADDR_I_ACCEL_FACTOR, g.i_accel_factor);
          display_all();
          // Five possible floating point values for acceleration
          set_accel_v();
          break;

        case 'B': // *B: Backlash compensation on / off
          g.backlash_on = 1 - g.backlash_on;
          update_backlash();
          display_all();
          EEPROM.put( ADDR_BACKLASH_ON, g.backlash_on);
          break;

        case 'C': // *C: Mirror lock: 0, 1, 2
          if (g.mirror_lock < 2)
            g.mirror_lock++;
          else
            g.mirror_lock = 0;
          display_all();
          EEPROM.put( ADDR_MIRROR_LOCK, g.mirror_lock);
          break;

        case 'D': // *D: temporarily disable limiters (not saved to EEPROM)
          g.disable_limiters = 1 - g.disable_limiters;
          display_all();
          break;

        case '4': // *4: Change N_timelapse
          if (g.i_n_timelapse < N_N_TIMELAPSE - 1)
            g.i_n_timelapse++;
          else
            g.i_n_timelapse = 0;
          EEPROM.put( ADDR_I_N_TIMELAPSE, g.i_n_timelapse);
          display_all();
          break;

        case '7': // *7: Change dt_timelapse
          if (g.i_dt_timelapse < N_DT_TIMELAPSE - 1)
            g.i_dt_timelapse++;
          else
            g.i_dt_timelapse = 0;
          EEPROM.put( ADDR_I_DT_TIMELAPSE, g.i_dt_timelapse);
          display_all();
          break;

        case '0': // *0: Save energy on / off
          g.save_energy = 1 - g.save_energy;
          update_save_energy();
          display_all();
          EEPROM.put( ADDR_SAVE_ENERGY, g.save_energy);
          break;

      } // switch
    }
  }

  //+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
  // Single-key commands (fake key events are allowed)
  else
  {
    if (state0_changed && (state0 == PRESSED || state0 == RELEASED))
    {

      if (state0 == PRESSED)
      {
        if (state0_changed && !fake_key)
        {
          // Memorizing the time when a key was pressed:
          // (Only when it is a true state change, not a fake key)
          g.t_key_pressed = g.t;
          if (g.calibrate_warning == 1)
            // Any key pressed when calibrate_warning=1 will initiate calibration:
          {
            g.calibrate_warning = 0;
            display_all();
            return;
          }
        }

        // Keys interpretation depends on the stacker_mode:
        if (g.stacker_mode == 0)
          // Mode 0: default; rewinding etc.
        {
          // When error 1 (limiter on initially), the only commands accepted are rewind and fast forward:
          if (g.error == 1 && key0 != '1' && key0 != 'A')
            return;

          switch (key0)
          {
            case '1':  // 1: Rewinding, or moving 10 frames back for the current stacking direction (if paused)
              if (g.pos_short_old <= g.limit1 && g.disable_limiters == 0 || g.paused > 1)
                break;
              if (g.paused)
              {
                if (g.moving)
                  break;
                frame_counter0 = g.frame_counter;
                g.frame_counter = g.frame_counter - 10;
                pos_target = frame_coordinate();
                if (pos_target < g.limit1 + (COORD_TYPE)100 || pos_target > g.limit2 - (COORD_TYPE)100 || g.paused && (g.frame_counter < 0 || g.frame_counter >= g.Nframes))
                {
                  g.frame_counter = frame_counter0;
                  break;
                }
                go_to(pos_target + 0.5, g.speed_limit);
                display_frame_counter();
              }
              else
              {
#ifdef EXTENDED_REWIND
                // Fixing a bug in extended rewind: disabling this feature if "1" was pressed when rail was moving
                if (g.moving || g.started_moving)
                  g.no_extended_rewind = 1;
#endif
                g.direction = -1;
                motion_status();
                // Rewinding is done with small acceleration:
                change_speed(-g.speed_limit, 0, 1);
              }
              break;

            case 'A':  // A: Fast forwarding, or moving 10 frames forward for the current stacking direction (if paused)
              if (g.pos_short_old >= g.limit2 && g.disable_limiters == 0 || g.paused > 1)
                break;
              if (g.paused)
              {
                if (g.moving)
                  break;
                frame_counter0 = g.frame_counter;
                g.frame_counter = g.frame_counter + 10;
                pos_target = frame_coordinate();
                if (pos_target < g.limit1 + (COORD_TYPE)100 || pos_target > g.limit2 - (COORD_TYPE)100 || g.paused && (g.frame_counter < 0 || g.frame_counter >= g.Nframes))
                {
                  g.frame_counter = frame_counter0;
                  break;
                }
                go_to(pos_target + 0.5, g.speed_limit);
                display_frame_counter();
              }
              else
              {
                g.direction = 1;
                motion_status();
                // Rewinding is done with small acceleration:
                change_speed(g.speed_limit, 0, 1);
              }
              break;

            case '4':  // 4: Set foreground point
              if (g.paused || g.moving)
                break;
              g.point1 = g.pos_short_old;
              EEPROM.put( ADDR_POINT1, g.point1);
              g.msteps_per_frame = Msteps_per_frame();
              g.Nframes = Nframes();
              points_status();
              display_two_point_params();
              display_two_points();
              display_comment_line(" Foreground point set");
              break;

            case 'B':  // B: Set background point
              if (g.paused || g.moving)
                break;
              g.point2 = g.pos_short_old;
              EEPROM.put( ADDR_POINT2, g.point2);
              g.msteps_per_frame = Msteps_per_frame();
              g.Nframes = Nframes();
              points_status();
              display_two_point_params();
              display_two_points();
              display_comment_line(" Background point set");
              break;

            case '7':  // 7: Go to the foreground point
              if (g.paused)
                break;
              go_to((float)g.point1 + 0.5, g.speed_limit);
              display_comment_line(" Going to Fore point ");
              break;

            case 'C':  // C: Go to the background point
              if (g.paused)
                break;
              go_to((float)g.point2 + 0.5, g.speed_limit);
              display_comment_line(" Going to Back point ");
              break;

            case '0': // 0: Start shooting (2-point focus stacking) from the foreground point (backlash compensated)
              if (g.moving)
                break;
              // Checking the correctness of point1/2
              if (g.point2 > g.point1 && g.point1 >= g.limit1 && g.point2 <= g.limit2)
              {
                if (g.paused == 1)
                  // Resuming 2-point stacking from a paused state
                {
                  g.paused = 0;
                  g.start_stacking = 1;
                  if (g.continuous_mode)
                  {
                    // The flag means we just initiated stacking:
                    letter_status(" ");
                  }
                  else
                  {
                    g.noncont_flag = 1;
                    letter_status("S");
                  }
                  // Time when stacking was initiated:
                  g.t0_stacking = g.t;
                  g.pos_to_shoot = g.pos_short_old;
                  g.stacker_mode = 2;
                }
                else if (g.paused == 3)
                  // Restarting from a pause which happened between stacks (in timelapse mode)
                {
                  g.paused = 0;
                  display_all();
                }
                else if (g.paused == 2)
                  // Restarting from a pause which happened during the initial travel to the starting point
                {
                  go_to((float)g.point1 + 0.5, g.speed_limit);
                  g.stacker_mode = 1;
                  g.start_stacking = 0;
                  g.paused = 0;
                  display_all();
                }
                else
                  // Initiating a new stack (or timelapse sequence of stacks)
                {
                  // Using the simplest approach which will result the last shot to always slightly undershoot
                  g.Nframes = Nframes();
                  go_to((float)g.point1 + 0.5, g.speed_limit);
                  g.starting_point = g.point1;
                  g.destination_point = g.point2;
                  g.stacker_mode = 1;
                  g.continuous_mode = 1;
                  g.start_stacking = 0;
                  g.timelapse_counter = 0;
                  if (N_TIMELAPSE[g.i_n_timelapse] > 1)
                    g.timelapse_mode = 1;
                  display_comment_line("   2-points stack    ");
                }
              }
              else
              {
                // Should print error message
                display_comment_line("      Bad 2 points!  ");
              }
              break;

            case '*':  // *: Show alternative display (for *X commands)
              if (g.paused)
                break;
              if (!g.moving && g.alt_flag == 0)
              {
                g.alt_flag = 1;
                display_all();
              }
              break;

            case 'D':  // D: Initiate one-point focus stacking forward
              if (g.paused)
                break;
              if (!g.moving)
              {
                // The flag means we just initiated stacking:
                g.start_stacking = 1;
                // Time when stacking was initiated:
                g.t0_stacking = g.t;
                g.frame_counter = 0;
                display_frame_counter();
                g.pos_to_shoot = g.pos_short_old;
                g.starting_point = g.pos_short_old;
                g.stacker_mode = 3;
                g.continuous_mode = 1;
                display_comment_line("    1-point stack    ");
              }
              break;

            case '2':  // 2: Decrease parameter n_shots (for 1-point sstacking)
              // Also used for different debugging modes, to decrease debugged parameters
              if (g.paused)
                break;
#ifndef BL_DEBUG
#ifndef BL2_DEBUG
#ifndef DELAY_DEBUG
              if (g.i_n_shots > 0)
                g.i_n_shots--;
              else
                break;
              EEPROM.put( ADDR_I_N_SHOTS, g.i_n_shots);
#else //DELAY_DEBUG
              // The meaning of "2" changes when DELAY_DEBUG is defined: now it is used to decrease the SHUTTER_ON_DELAY2 parameter:
              SHUTTER_ON_DELAY2 = SHUTTER_ON_DELAY2 - DELAY_STEP;
              if (SHUTTER_ON_DELAY2 < 0)
                SHUTTER_ON_DELAY2 = 0;
#endif // DELAY_DEBUG              
#else // BL2_DEBUG
              // The meaning of "2" changes when BL2_DEBUG is defined: now it is used to decrease the BACKLASH_2 parameter:
              BACKLASH_2 = BACKLASH_2 - BL_STEP;
              if (BACKLASH_2 < 0)
                BACKLASH_2 = 0;
#endif //BL2_DEBUG
#else // BL_DEBUG
              // The meaning of "2" changes when BL_DEBUG is defined: now it is used to decrease the g.backlash parameter:
              g.backlash = g.backlash - BL_STEP;
              if (g.backlash < 1)
                g.backlash = 1;
#endif // BL_DEBUG
              display_all();
              break;

            case '3':  // 3: Increase parameter n_shots (for 1-point sstacking)
              // Also used for different debugging modes, to increase debugged parameters
              if (g.paused)
                break;
#ifndef BL_DEBUG
#ifndef BL2_DEBUG
#ifndef DELAY_DEBUG
              if (g.i_n_shots < N_PARAMS - 1)
                g.i_n_shots++;
              else
                break;
              EEPROM.put( ADDR_I_N_SHOTS, g.i_n_shots);
#else //DELAY_DEBUG
              // The meaning of "3" changes when DELAY_DEBUG is defined: now it is used to increase the SHUTTER_ON_DELAY2 parameter:
              SHUTTER_ON_DELAY2 = SHUTTER_ON_DELAY2 + DELAY_STEP;
              if (SHUTTER_ON_DELAY2 > 10000000)
                SHUTTER_ON_DELAY2 = 10000000;
#endif // DELAY_DEBUG              
#else // BL2_DEBUG
              // The meaning of "3" changes when BL2_DEBUG is defined: now it is used to increase the BACKLASH_2 parameter:
              BACKLASH_2 = BACKLASH_2 + BL_STEP;
              if (BACKLASH_2 > 10000)
                BACKLASH_2 = 10000;
#endif // BL2_DEBUG
#else // BL_DEBUG
              // The meaning of "3" changes when BL_DEBUG is defined: now it is used to increase the g.backlash parameter:
              g.backlash = g.backlash + BL_STEP;
              if (g.backlash > 10000)
                g.backlash = 10000;
#endif // BL_DEBUG
              display_all();
              break;

            case '5':  // 5: Decrease parameter mm_per_frame
              if (g.paused)
                break;
              if (g.i_mm_per_frame > 0)
                g.i_mm_per_frame--;
              else
                break;
              // Required microsteps per frame:
              g.msteps_per_frame = Msteps_per_frame();
              g.Nframes = Nframes();
              if (g.Nframes > 9999)
                // Too many frames; recovering the old values
              {
                g.i_mm_per_frame++;
                g.msteps_per_frame = Msteps_per_frame();
                g.Nframes = Nframes();
                break;
              }
              display_all();
              EEPROM.put( ADDR_I_MM_PER_FRAME, g.i_mm_per_frame);
              break;

            case '6':  // 6: Increase parameter mm_per_frame
              if (g.paused)
                break;
              if (g.i_mm_per_frame < N_PARAMS - 1)
              {
                g.i_mm_per_frame++;
                // Estimating the required speed in microsteps per microsecond
                speed = target_speed();
                // Reverting back if required speed > maximum allowed:
                if (speed > g.speed_limit)
                {
                  g.i_mm_per_frame--;
                  break;
                }
              }
              else
                break;
              // Required microsteps per frame:
              g.msteps_per_frame = Msteps_per_frame();
              g.Nframes = Nframes();
              EEPROM.put( ADDR_I_MM_PER_FRAME, g.i_mm_per_frame);
              display_all();
              break;

            case '8':  // 8: Decrease parameter fps
              if (g.paused)
                break;
              if (g.i_fps > 0)
                g.i_fps--;
              else
                break;
              EEPROM.put( ADDR_I_FPS, g.i_fps);
              display_all();
              break;

            case '9':  // 9: Increase parameter fps
              if (g.paused)
                break;
              if (g.i_fps < N_PARAMS - 1)
              {
                g.i_fps++;
                // Estimating the required speed in microsteps per microsecond
                speed = target_speed();
                // Reverting back if required speed > maximum allowed:
                if (speed > g.speed_limit || FPS[g.i_fps] > MAXIMUM_FPS)
                {
                  g.i_fps--;
                  break;
                }
              }
              else
                break;
              EEPROM.put( ADDR_I_FPS, g.i_fps);
              display_all();
              break;

            case '#': // #: Show the non-continuous parameters in the 5th line of the LCD (Row 4)
              if (g.moving || g.paused)
                break;
              delay_buffer();
              display_comment_line(g.buffer);
              break;

          } // End of case
        }  // if (g.stacker_mode == 0)
        //+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
        // End of processing specific key presses

        else if (g.stacker_mode > 0)
          // Mode 1/2: focus stacking
        {
          // Any key press in stacking mode interrupts stacking
          if (g.moving)
            change_speed(0.0, 0, 2);
          // All situations when we pause: during 2-point stacking, while travelling to the starting point, and while waiting between stacks in timelaspe mode:
          if (g.stacker_mode == 2 || g.stacker_mode == 1 || g.stacker_mode == 4)
          {
            // Paused during 2-point stacking
            if (g.stacker_mode == 2)
            {
              g.paused = 1;
              if (g.t - g.t0_stacking > CONT_STACKING_DELAY)
              {
                // Switches the frame counter back to the last accomplished frame
                g.frame_counter--;
                // I think this is the logical behaviour: when paused between two frame positions, instantly rewind to the last taken frame position:
                pos_target = frame_coordinate();
                go_to(pos_target + 0.5, g.speed_limit);
              }
            }
            // Paused while travelling to the starting point
            else if (g.stacker_mode == 1)
            {
              g.paused = 2;
            }
            // Paused while waiting between stacks in timelaspe mode
            else
            {
              g.paused = 3;
            }
            display_comment_line("    Paused    ");
            letter_status("P");
          }
          else
            // In 1-point stacking, we abort
          {
            g.frame_counter = 0;
            display_comment_line("  Stacking abort     ");
          }
          g.stacker_mode = 0;
        }

      }  // if (PRESSED)

      //+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
      else
        // if a key was just released
      {
        // Resetting the counter of key repeats:
        g.N_repeats = 0;
        // Breaking / stopping if no keys pressed (only after rewind/fastforward)
        if ((g.key_old == '1' || g.key_old == 'A') && g.moving == 1 && state0 == RELEASED && state0_changed && g.paused == 0)
        {
#ifdef EXTENDED_REWIND
          if (g.key_old == '1' && g.speed < 0.0 && g.backlash_on && !g.no_extended_rewind)
            // Moving in the bad (negative) direction
          {
            // Estimating how much rail would travel if the maximum breaking started now (that's how much
            // rail would actually travel if moving in the good direction, or if backlash was zero):
            // Stopping distance in the current direction:
            float dx_stop = g.speed * g.speed / (2.0 * ACCEL_LIMIT);
            // The physical coordinate where we have to stop:
            float pos1 = g.pos - dx_stop;
            // To mimick the good direction (key "A") behaviour, we replace emergency breaking with a go_to call:
            // (All technicalities - backlash compensation, limit of decceleration - will be handled by go_to)
            go_to(pos1, g.speed_limit);
          }
          else
#endif
            change_speed(0.0, 0, 2);
        }
        if (g.key_old == '*')
          // The '*' key was just released: switch to default screen from the alternative one
        {
          g.alt_flag = 0;
          display_all();
        }
      }

    }  // End of if(keyStateChanged)
  } // End of two-key / one-key if


  return;
}
