//#######################################################################################################
//######################## Plugin 201: Temperature and Level sensor xHB01 ########################
//#######################################################################################################

#define PLUGIN_201
#define PLUGIN_ID_201         201
#define PLUGIN_NAME_201       "Environment - xHB01"
#define PLUGIN_VALUENAME1_201 "Temperature"
#define PLUGIN_VALUENAME2_201 "Level"

// Extracted from http://hack4life.pbworks.com/Arduino%20Solar%20Water%20Heater%20Sensor

#define TIMEOUT 50000
#define MARK_HEADER 7000
#define SPACE_HEADER 3000
#define MARK 600
#define SPACE_ONE 1600
#define SPACE_ZERO 600

int expectPulse(int val, int inPin){
  unsigned long t=micros();
  while(digitalRead(inPin)==val){
    if( (micros()-t)>TIMEOUT ) return 0;
  }
  return micros()-t;
}

// temp in celsious and level goes from 0 to 3
bool readTempNLevelSensor(char inPin, char &temp, char &level, byte &checksum){
   byte data[5]={0,0,0,0,0};
   unsigned long val1;
   unsigned long st=micros();
   val1 = expectPulse(HIGH, inPin);     //Mide un pico; si resulta ser un valle, dará un return muy breve que no cumple lo siguiente, y salta a return false
   if(val1>MARK_HEADER){         //Si hay un pico mayor que el pico de cabecera (si no, salta a return false)
     val1 = expectPulse(LOW, inPin);    //Lee un valle
     if(val1>SPACE_HEADER){      //Y si el valle es mayor que el valle de cabecera (si no, salta a return false)
       int c=39;
       for(;c>-1;c--){           //Comienza el bucle de lectura de 40 bits
         val1 = expectPulse(HIGH, inPin); //Lee un pico
         if(val1<MARK){            //Si el pico es más corto que MARK
          val1 = expectPulse(LOW, inPin);
          if(val1==0){             //Y el valle siguiente da timeout
            //Serial.println("Mark error 0");
            break;                //Error y para
          }
          if(val1<SPACE_ZERO){   //Si el valle es menos que un valle de "0" (600)
            //0
            //data[c>>3]|=(1<<(c & B111));
          }else if(val1<SPACE_ONE){ //Si no, si es más pequeño que un valle de "1" (1500)
            //1
            data[c>>3]|=(1<<(c & B111)); //Es un "1" y lo guarda (como la matriz ya estaba llena de ceros, no hace falta escribir un "0" si no es un "1")
          }else{
           //Serial.print(val1);Serial.println(" Space error");
           break;
          }
         }else{
          //Serial.println("Mark error");
          break;
         }
       }
       // Each reading should not take more than 70ms (use time to detect errors)
       if(micros()-st<70000){
         temp=data[3];
         level=data[2];
         checksum=data[0];
         //Serial.print(data[3]);Serial.print(" ");Serial.println((data[2]));
         //Serial.print(data[4],HEX);Serial.print(" ");Serial.print(data[3],HEX);Serial.print(" ");Serial.print(data[2],HEX);Serial.print(" ");Serial.print(data[1],HEX);Serial.print(" ");Serial.println(data[0],HEX);
         //Serial.println((216-temp-level),HEX);
         return true;
       }
    }
   }
   return false;
}


boolean Plugin_201(byte function, struct EventStruct *event, String& string)
{
  boolean success = false;

int inPin = 14; //Settings.TaskDevicePin1[event->TaskIndex]; //Hardcoded 14 porque con la referencia original da reseteos al acceder a la config. del dispositivo en el interfaz web
pinMode(inPin, INPUT); //puesto aquí porque fuera no funciona

  switch (function)
  {
    case PLUGIN_DEVICE_ADD:
      {
        Device[++deviceCount].Number = PLUGIN_ID_201;
        Device[deviceCount].Type = DEVICE_TYPE_SINGLE;
        Device[deviceCount].VType = SENSOR_TYPE_TEMP_HUM;
        Device[deviceCount].Ports = 0;
        Device[deviceCount].PullUpOption = false;
        Device[deviceCount].InverseLogicOption = false;
        Device[deviceCount].FormulaOption = true;
        Device[deviceCount].ValueCount = 2;
        Device[deviceCount].SendDataOption = true;
        Device[deviceCount].TimerOption = true;
        Device[deviceCount].GlobalSyncOption = true;
        break;
      }

    case PLUGIN_GET_DEVICENAME:
      {
        string = F(PLUGIN_NAME_201);
        break;
      }

    case PLUGIN_GET_DEVICEVALUENAMES:
      {
        strcpy_P(ExtraTaskSettings.TaskDeviceValueNames[0], PSTR(PLUGIN_VALUENAME1_201));
        strcpy_P(ExtraTaskSettings.TaskDeviceValueNames[1], PSTR(PLUGIN_VALUENAME2_201));
        break;
      }

    case PLUGIN_WEBFORM_LOAD:
      {
        /*
        const String options[] = { F("DHT 11"), F("DHT 22"), F("DHT 12") };
        int indices[] = { 11, 22, 12 };

        addFormSelector(string, F("DHT Type"), F("plugin_201_dhttype"), 3, options, indices, Settings.TaskDevicePluginConfig[event->TaskIndex][0] );
        */
        success = true;
        break;

      }

    case PLUGIN_WEBFORM_SAVE:
      {
/*
        Settings.TaskDevicePluginConfig[event->TaskIndex][0] = getFormItemInt(F("plugin_201_dhttype"));
*/
        success = true;
        break;

      }

    //case PLUGIN_TEN_PER_SECOND:
    //case PLUGIN_ONCE_A_SECOND:
    case PLUGIN_READ:
      {
        char temp,level;
        byte checksum;
        unsigned long t0 = micros();
        while(!readTempNLevelSensor(inPin, temp, level, checksum)){
           //Mientras no encuentre una cabecera, repite hasta encontrarla y entonces sal del while y define la temperatura y nivel
            if ((micros()-t0) > 1000000)                  //Pero no más de x segundos (para evitar un bucle infinito si falla el sensor y para que no ocupe mucho procesador)
            {                                             //Como los paquetes se mandan cada 940ms
              temp = 0;
              level = 0;
              break;
            }
          }

        if (temp!=0 || level!=0)
        {
            if (216-temp-level == checksum) //comprobación con fórmula averiguada para el checksum (quinto byte)
            {
              float watertemperature = temp; //temperatura que se enviará a Domoticz
              float waterlevel = 25*level; //el nivel es un valor entero de 0 a 4, así que se pasa a porcentaje (0-25-50-75-100)



        if (watertemperature != NAN || waterlevel != NAN) // According to negated original if, maybe use && instead?
                {
                  //Serial.print(watertemperature,DEC);Serial.print("c ");Serial.println(waterlevel,DEC);
                  UserVar[event->BaseVarIndex] = watertemperature;
                  UserVar[event->BaseVarIndex + 1] = waterlevel;
                  String log = F("xHB01  : Temperature: ");
                  log += UserVar[event->BaseVarIndex];
                  addLog(LOG_LEVEL_INFO, log);
                  log = F("xHB01  : Level: ");
                  log += UserVar[event->BaseVarIndex + 1];
                  addLog(LOG_LEVEL_INFO, log);
                  success = true;
                }
            } //checksum

        }// temp y level !=0
      } //PLUGIN_ONCE_A_SECOND
  }
  return success;
}
