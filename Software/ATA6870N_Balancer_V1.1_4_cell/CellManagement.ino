 
 // adc to voltage conversion.
 
 // calibration.
 
 // algorithm for balancing
 
 // Balance algotithim 1:
 // find lowest cell and discharge all other cellls within XXX mv of that cell
 
 // Balance algorithim 2:
 // measure cell voltage every minute or so and calculate slope and time till full. start discharging cells that have have lower "time till full" values.
 // return estimated charge time in minutes. give target voltage and weather to actually write cells to be discharged (also useful for estimating charging time)
 
 
 // 
 
 // highest and lowest voltages of cells in the stack. global variables.
 //float maxvoltage; - moved to main sketch
 //float minvoltage;
 
 void CalcDeciVolts(float *cellVoltages, uint16_t *cellReadings, uint16_t *cellCalVals )// calculate floating point voltages from adc outputs.
 {
  // calculate decimal cell voltages
  for(int cellNum = 0; cellNum < (BALANCERCOUNT * 6); cellNum++)
  {
   cellVoltages[cellNum] = float(cellReadings[cellNum] - cellCalVals[cellNum]) * 4 / (3031 - cellCalVals[cellNum]);
  } 
 }
 
 
// function to calculate maximum and minimum voltages
float CalcExtremes(float *cellVoltages)
{ // revise to ignore unconnected cells!
  minvoltage = cellVoltages[0]; //initalise minvoltage with somthing not "0"
  maxvoltage = cellVoltages[0]; //initalise maxvoltage from a new cell voltage value (old value could be greater than current values)
  
  for(int cellNum = 0; cellNum < (BALANCERCOUNT * 6); cellNum++)
    if(UNCONNECTED_CELLS[0] & (0x01 << cellNum) != 0) // check to make sure we should calculate this cell. fix this
    {
      if(cellVoltages[cellNum] < minvoltage)// find lowest voltage cell
      {
        minvoltage = cellVoltages[cellNum]; // record voltage
      }
      else if(cellVoltages[cellNum] > maxvoltage)// find highest voltage cells
      {
        maxvoltage = cellVoltages[cellNum]; // record voltage
      }
    }
  
  Serial.print(F("Minvoltage = ")); 
  Serial.println(minvoltage, 3);

  Serial.print(F("Maxvoltage = ")); 
  Serial.println(maxvoltage, 3);

  
  return minvoltage; 
}

float CalcTotalVolts(float *cellVoltages) // calculate total pack voltage
{
  
 float totalVolts;
 
   for(int cellNum = 0; cellNum < (BALANCERCOUNT * 6); cellNum++)
  {
   totalVolts = totalVolts +  cellVoltages[cellNum];
  }
 return totalVolts; 
}
 ////////////////////////////////////////
// function to compare the voltages of the cells and balance them
////////////////////////////////////////
int CalcBalanceCells(float *cellVoltages, byte *balanceStates, byte *ignoredCells, float balanceVoltage)//calculate cells to be balanced.
{ 
  int numOfActiveCells; // number of cells that are turned on. includes unconnected calls (which are always on)
  
  Serial.print(F("balancing to "));
  Serial.println(balanceVoltage, 3);
  
  for(int deviceNum = 0; deviceNum < BALANCERCOUNT; deviceNum++) // iterate through connected boards. 
    {
      for (int cellNum = 0; cellNum < 6; cellNum++) // iterate through cells by bitshifting active cell until the "1" reaches the 6th bit.
      { 
        uint8_t cellMask = (0x01 << cellNum);
        
        if(ignoredCells[deviceNum] & cellMask != 0) // check to make sure we should calculate this cell.
        { 
          Serial.print(F("cell#"));
          Serial.print(cellNum);
          Serial.print(F(" on dev#"));
          Serial.print(deviceNum);
          Serial.print(F(" of voltage "));
          Serial.print(cellVoltages[(deviceNum * 6) + cellNum], 3);
          
          if(cellVoltages[(deviceNum * 6) + cellNum] >= balanceVoltage)
          { 
            
            Serial.println(" is on");
            
            balanceStates[deviceNum] = balanceStates[deviceNum] | cellMask;
            numOfActiveCells++;
          } 
          else 
          {  
            balanceStates[deviceNum] = balanceStates[deviceNum] & ~cellMask; // zero out off cells with inverted cellmask
            Serial.println(" is off");
          }
        }
       else
      {
        //numOfActiveCells++; // count inactive cells - balance function will actually turn them on.
      } 
      } // end cell for
    }// end device for
  return numOfActiveCells;
}




void balance(byte *balanceStates, byte *ignoredCells, boolean enSel)// takes balanceStates and writes them to the balancer boards
{ // enSel = weather to enable or disable balance cells. used for pwming in thermal management. if o cells disabled, if 1 cells are enabled and can balance. "unconnected cells" are always on.

  for(int deviceNum = 0; deviceNum < BALANCERCOUNT; deviceNum++) // iterate through connected boards. 
    { 
      byte drain = 0; // place to store active balancer data
    
      for (uint8_t cellMask = 0x01; cellMask < 64; cellMask <<= 1) // iterate through cells by bitshifting active cell until the "1" reaches the 6th bit.
      {     
        if(ignoredCells[deviceNum] & cellMask != 0) // check if it's an active cell
        {
          drain = drain | ((balanceStates[deviceNum] & cellMask) * enSel);
        }
        else // if it's not an active cell, turn it on.
        { 
          drain = drain | cellMask;
        }
        
        drain = drain & B00001111 ;
        ATA68_ResistorControl(deviceNum, drain); // send command
      }
    }
}

void TestCellLoads()
{
     // /*
   // test cell balance loads.
   for(int i=0; i < BALANCERCOUNT ; i++) // iterate through connected boards. 
    {
      for (uint8_t drain = 0x01; drain <= 64; drain <<= 1) // iterate through cells by bitshifting active cell until the "1" reaches the 6th bit.
      {
        ATA68_ResistorControl(i, drain); // send command
        
        delay(200); // allow time to observe tap resistor activating.
      }
      ATA68_ResistorControl(i, 0x00); // turn off all cells
    }
   // */
    
    //ATA68_ResistorControl(0, B00101001); // turn on all cells for that board.
    //ATA68_ResistorControl(1, B00111111); // turn on all cells for that board. 
}


int calcAverageInt(int newdata, int *oldDataArray, int sampleNumber)// calculates average over time through a scrollling database. keeps specified number of samples to average and discards the oldest one every time
{// nedata = data that was just recorded
 // oldDataArray = array of old data. user should not touch this and is completely handled by this function
 // sampleNumber = number of samples to average. 1 = complete passthrough, 2 = last two samples (one in array, one in newData)
 // warning, may overflow int if samples get too large or samplenumber is too large.
 // will also corrupt memory if array and samplenumber are the wrong size.
 
 int average;
 
 for(int i = 0; i < sampleNumber; i++) // shift values once to the right
 {
   oldDataArray[i] = oldDataArray[i+1];
   
   if(i == 0)
   {
     oldDataArray[0] = newdata; // add newdata to data array
   }
   
   average = oldDataArray[i] + average;
 }
 
  average = average / sampleNumber;
 
  return average; 
}
