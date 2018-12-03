class ClassCorriente {
  private:
    float SENSIBILITY;
    int SAMPLESNUMBER;
  public:
  void setup(){
    // Sensibilidad del sensor en V/A
    //SENSIBILITY = 0.185;   // Modelo 5A
    //SENSIBILITY = 0.100; // Modelo 20A
    SENSIBILITY = 0.066; // Modelo 30A
    SAMPLESNUMBER = 100;
  }
  float getCorriente(int samplesNumber){
     float voltage;
     float corrienteSum = 0;
     for (int i = 0; i < samplesNumber; i++)
     {
        voltage = analogRead(A0) * 5.0 / 1023.0;
        corrienteSum += (voltage - 2.5) / SENSIBILITY;
     }
     return(corrienteSum / samplesNumber);
  };
  float voltaje(){
    float voltage = analogRead(A0) *(5.0 / 1023.0);
    return voltage;
  }
  float intensidad(){
    return getCorriente(SAMPLESNUMBER);
  };
  float irms(){
    float current = getCorriente(SAMPLESNUMBER);
    float currentRMS = 0.707 * current;
    return currentRMS;
  };
  float potencia(){
    float current = getCorriente(SAMPLESNUMBER);
    float currentRMS = 0.707 * current;
    float power = 230.0 * currentRMS;
    return power;
  };
  String toString(){
    return "V= "+String(voltaje())+" A= "+String(intensidad())+" P= "+String(potencia());
  };
};
ClassCorriente Corriente;
