#include "mbed.h"
#include "Adafruit_SSD1306.h"
#include "array"

DigitalOut led(LED1);
DigitalOut RCServo_Status_Check_LED(D2); //
InterruptIn Lift_SW(D4,PullUp);
PwmOut RCServo(D6);
PwmOut speaker(D12);
I2C i2c(I2C_SDA, I2C_SCL);
Adafruit_SSD1306_I2c myOled(i2c,D4,0x78,32,128);
void turn(PwmOut &rc,float deg);
template <class T>
T map(T x, T in_min, T in_max, T out_min, T out_max);
enum RCSERVO_FLAG {
    ZERO_TO_ONE_EIGHTY,
    ONE_EIGHTY_TO_ZERO,
};
void Lifter_Project(int& rcservo_direction_flag, int& rcservo_axis_count);
enum LiftState {
    SHRINK,
    UP,
    EXTEND,
    DOWN,
    RETURN_SHRINK,
    RETURN_EXTEND
};
static const char *liftState_str[] =
        { "SHRINK    ", "GOING UP  ", "EXTEND    ", "GOING DOWN", "SHRINK    ", "EXTEND   " };
int mode = SHRINK;

class Lift {
    private:
        int angle = 0;
        int delay_us = 20000;

        void shrink_mode() {
            RCServo_Status_Check_LED = 0;
            turn(RCServo,0);
            angle=0;
            mode=SHRINK;
        }

        void up_mode() {
            if(angle >= 180.f) {
                RCServo_Status_Check_LED = 1;
                mode=EXTEND;
                return ;
            };
            turn(RCServo,angle);
            angle+=1;
            wait_us(delay_us);
            if(angle%5==0) RCServo_Status_Check_LED=!RCServo_Status_Check_LED;
        }

        void extend_mode() {
            RCServo_Status_Check_LED = 1;
            turn(RCServo,180);
            angle=180;
            mode=EXTEND;
        }

        void down_mode() {
            if(angle <= 0.f) {
                RCServo_Status_Check_LED = 0;
                mode=SHRINK;
                return ;
            };
            turn(RCServo,angle);
            angle-=1;
            wait_us(delay_us);
            if(angle%5==0) RCServo_Status_Check_LED=!RCServo_Status_Check_LED;
        }
    public:
        void move(int mode) {
            switch(mode) {
                case SHRINK:
                    shrink_mode();
                    break;
                case UP: 
                    up_mode();
                    break;
                case EXTEND: 
                    extend_mode();
                    break;
                case DOWN: 
                    down_mode();
                    break;
            }
        }
        
};

// 스위치를 눌렀을 떄 부저소리와 함께 모드변경
void change_mode() {
    speaker.write(0.2);
    speaker.period_us(100000/262);
    wait_us(100000);
    speaker.write(0);
    switch(mode) {
        case SHRINK:
            mode = UP;
            break;
        case UP:
            mode = SHRINK;
            break;
        case EXTEND: 
            mode = DOWN;
            break;
        case DOWN: 
            mode = EXTEND;
            break;
    }   
}

int main()
{
    RCServo.period_ms(20); // 50khz
    i2c.frequency(400000);
    myOled.begin();
    myOled.clearDisplay();
    Lift lift = Lift();
    Lift_SW.fall(&change_mode);
    
    while (true) {
        lift.move(mode);
        myOled.printf("%s\r",liftState_str[mode]);
        myOled.display();
    }
}
void turn(PwmOut &rc, float deg) {
	uint16_t pulseW = map<float>(deg,0.,180.,600.,2400.);
	rc.pulsewidth_us(pulseW);
}
template <class T>
T map(T x, T in_min, T in_max, T out_min, T out_max) {
	return (x - in_min) * (out_max - out_min) / (in_max - in_min) + out_min;
}
