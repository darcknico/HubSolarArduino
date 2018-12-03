DS3231  rtc;

class RelojClass {
  private:
    bool Century=false;
    bool h12;
    bool PM;
  public:

  void setup(){
    setTime(rtc.getHour(h12, PM),rtc.getMinute(),rtc.getSecond(),rtc.getMonth(Century),rtc.getDate(),rtc.getYear());
  }

  String hora(String separador){
    String horas = String(rtc.getHour(h12, PM));
    if(rtc.getHour(h12, PM)<10){
      horas = "0"+horas;
    }
    String minutos = String(rtc.getMinute());
    if(rtc.getMinute()<10){
      minutos = "0"+minutos;
    }
    String segundos = String(rtc.getSecond());
    if(rtc.getSecond()<10){
      segundos = "0"+segundos;
    }
    return horas+separador+minutos+separador+segundos;
  }

  String hora(){
    String horas = String(rtc.getHour(h12, PM));
    if(rtc.getHour(h12, PM)<10){
      horas = "0"+horas;
    }
    String minutos = String(rtc.getMinute());
    if(rtc.getMinute()<10){
      minutos = "0"+minutos;
    }
    String segundos = String(rtc.getSecond());
    if(rtc.getSecond()<10){
      segundos = "0"+segundos;
    }
    return horas+":"+minutos+":"+segundos;
  }

  String fecha(String separador){
    String dias = String(rtc.getDate());
    if(rtc.getDate()<10){
     dias = "0"+dias; 
    }
    String meses = String(rtc.getMonth(Century));
    if(rtc.getMonth(Century)<10){
      meses = "0"+meses;
    }
    return dias+separador+meses+separador+rtc.getYear();
  };

  String fecha(){
    String dias = String(rtc.getDate());
    if(rtc.getDate()<10){
     dias = "0"+dias; 
    }
    String meses = String(rtc.getMonth(Century));
    if(rtc.getMonth(Century)<10){
      meses = "0"+meses;
    }
    return dias+"/"+meses+"/"+rtc.getYear();
  };
};
RelojClass Reloj;

