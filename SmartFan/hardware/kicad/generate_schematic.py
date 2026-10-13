from pathlib import Path
import uuid,json,sys
out=Path(sys.argv[1]);out.mkdir(parents=True,exist_ok=True)
uid=lambda:str(uuid.uuid4())
q=lambda s:json.dumps(s,ensure_ascii=False)
root=uid();libs=[];items=[];nets=[]
font='(effects (font (size 1.27 1.27)))'
def text(s,x,y,size=1.5):
 items.append(f'(text {q(s)} (at {x} {y} 0) (effects (font (size {size} {size})) (justify left)) (uuid {uid()}))')
def library(name,pins,w,h,ref='J',graphics=''):
 shapes=f'(rectangle (start {-w/2} {h/2}) (end {w/2} {-h/2}) (stroke (width 0.254) (type default)) (fill (type background)))'
 pinstr=''
 for n,label,px,py,angle in pins:
  pinstr+=f'(pin passive line (at {px} {py} {angle}) (length 5) (name {q(label)} {font}) (number {q(str(n))} {font}))'
 libs.append(f'(symbol "SmartFan:{name}" (pin_names (offset 1)) (in_bom yes) (on_board yes) (property "Reference" "{ref}" (at 0 {h/2+4} 0) {font}) (property "Value" {q(name)} (at 0 {-h/2-4} 0) {font}) (symbol "{name}_0_1" {shapes} {graphics}) (symbol "{name}_1_1" {pinstr}))')
 return name,pins,w,h

def instance(lib,ref,value,x,y,mapping):
 name,pins,w,h=lib
 u=uid()
 pstr=''.join(f'(pin "{n}" (uuid {uid()}))' for n,*_ in pins)
 items.append(f'(symbol (lib_id "SmartFan:{name}") (at {x} {y} 0) (unit 1) (in_bom yes) (on_board yes) (dnp no) (uuid {u}) (property "Reference" {q(ref)} (at {x} {y-(3 if ref.startswith('D') else h/2+5)} 0) {font}) (property "Value" {q(value)} (at {x} {y+(3 if ref.startswith('D') else h/2+5)} 0) {font}) {pstr} (instances (project "SmartFan" (path "/{root}" (reference {q(ref)}) (unit 1)))))')
 for n,label,px,py,angle in pins:
  ax=x+px;ay=y-py
  if n not in mapping:
   items.append(f'(no_connect (at {ax} {ay}) (uuid {uid()}))');continue
  net=mapping[n];bx=ax+(-10 if angle==0 else 10)
  items.append(f'(wire (pts (xy {ax} {ay}) (xy {bx} {ay})) (stroke (width 0) (type default)) (uuid {uid()}))')
  rot=0 if angle==0 else 180
  items.append(f'(label {q(net)} (at {bx} {ay} {rot}) (effects (font (size 1.1 1.1)) (justify left bottom)) (uuid {uid()}))')
  nets.append((ref,n,net))

def connector(name,labels):
 h=(len(labels)-1)*5+10;w=30
 pins=[(i+1,s,-w/2-5,(len(labels)-1)*2.5-i*5,0) for i,s in enumerate(labels)]
 return library(name,pins,w,h)

pins=[]
for i in range(20):
 pins += [(2*i+1,str(2*i+1),-23,47.5-i*5,0),(2*i+2,str(2*i+2),23,47.5-i*5,180)]
j=library('JETSON_J12_BOARD',pins,36,105)
board={1:'V3V3',2:'V5V_BREADBOARD',6:'GND',7:'LED1_A',11:'US_TRIG',12:'LED2_A',13:'ENC_S1',16:'LED4_A',18:'ENC_S2',19:'LED3_A',21:'LED5_A',22:'ENC_KEY',23:'LED6_A',29:'MOTOR_IN1',31:'LED8_A',33:'MOTOR_ENA_PWM',36:'US_ECHO_DIRECT',37:'LED7_A'}
instance(j,'J1','Orin Nano J12 - PHYSICAL pin numbers',65,105,board)
enc=connector('ENCODER_MODULE',['S1','S2','KEY','VCC','GND'])
instance(enc,'J2','Encoder - verify VCC / pin order',180,60,{1:'ENC_S1',2:'ENC_S2',3:'ENC_KEY',4:'ENC_VCC_TBD',5:'GND'})
us=connector('ULTRASONIC_MODULE',['VCC','TRIG','ECHO','GND'])
instance(us,'J3','Ultrasonic - model TBD',180,112,{1:'US_VCC_TBD',2:'US_TRIG',3:'US_ECHO_DIRECT',4:'GND'})
motor=connector('MOTOR_DRIVER_MODULE',['ENA','IN1','IN2','VMOTOR','GND','OUT1','OUT2'])
instance(motor,'J5','HW-095 - power terminal / pin order TBD',295,55,{1:'MOTOR_ENA_PWM',2:'MOTOR_IN1',3:'GND',4:'V5V_BREADBOARD',5:'GND',6:'MOTOR_OUT1',7:'MOTOR_OUT2'})
m=connector('DC_MOTOR',['TERMINAL_1','TERMINAL_2'])
instance(m,'M1','EZmotor R300',295,140,{1:'MOTOR_OUT1',2:'MOTOR_OUT2'})
rn_pins=[(1,'COM',22.5,0,180)]
rn_graphics=''
def line(x1,y1,x2,y2):
 return f'(polyline (pts (xy {x1} {y1}) (xy {x2} {y2})) (stroke (width 0.127) (type default)) (fill (type none)))'
for i in range(8):
 py=35-i*10
 rn_pins.append((i+2,f'R{i+1}',-22.5,py,0))
 rn_graphics+=line(-17.5,py,-9,py)+line(-3,py,8,py)
 rn_graphics+=f'(rectangle (start -9 {py+1.2}) (end -3 {py-1.2}) (stroke (width 0.127) (type default)) (fill (type none)))'
rn_graphics+=line(8,35,8,-35)+line(8,0,17.5,0)
rn=library('RN_9A331G_BUSSED',rn_pins,35,80,'RN',rn_graphics)
instance(rn,'RN1','9A331G - 8 x 330R',240,210,
         {1:'GND',**{i+2:f'LED{i+1}_K' for i in range(8)}})
d=library('LED_SEGMENT',[(1,'A',-10,0,0),(2,'K',10,0,180)],10,5,'D')
for i in range(8):
 y=175+i*10
 instance(d,f'D{i+1}',f'LED{i+1}',170,y,{1:f'LED{i+1}_A',2:f'LED{i+1}_K'})
text('SMART FAN - FUNCTIONAL WIRING SCHEMATIC',20,15,3)
text('ENA = BOARD 33 / 32c0000.pwm     IN1 = BOARD 29',20,25)
text('Module symbols show SIGNAL FUNCTIONS, not verified manufacturer pin numbers.',20,35)
text('LED1..8: BOARD 7 / 12 / 19 / 16 / 21 / 23 / 37 / 31',20,265)
text('LED output: OFF=0, S1=2, S2=5, S3=8 segments.',20,275)
body=f'(kicad_sch (version 20250114) (generator "eeschema") (uuid {root}) (paper "A3") (lib_symbols '+''.join(libs)+') '+''.join(items)+' (sheet_instances (path "/" (page "1"))))'
(out/'SmartFan.kicad_sch').write_text(body,encoding='utf-8')
(out/'SmartFan.kicad_pro').write_text(json.dumps({'meta':{'filename':'SmartFan.kicad_pro','version':1}},indent=2),encoding='utf-8')
(out/'connections.csv').write_text('Reference,Pin,Net\n'+''.join(f'{a},{b},{c}\n' for a,b,c in nets),encoding='utf-8')
(out/'README.md').write_text('''# Smart Fan KiCad 회로도 초안

SmartFan.kicad_pro 또는 SmartFan.kicad_sch를 KiCad에서 열면 됩니다.
심볼을 파일 안에 포함하여 별도 라이브러리 설치가 필요 없습니다. PCB 배치는 포함하지 않습니다.

확정 배선: Jetson 물리 핀 번호 기준.
- 초음파 Trig=11 / Echo=36
- 엔코더 S1=13 / S2=18 / KEY=22
- 모터 ENA=33 / IN1=29 / IN2=GND
- LED1..8=7/12/19/16/21/23/37/31

J2/J3/J5의 번호는 기능 설명용이며 실제 모듈 단자 번호가 아닙니다.
모델과 데이터시트 확인 없이 PCB footprint로 연결하지 마세요.
M1은 사용자 확인 모델 EZmotor R300입니다.
J5 모터 드라이버는 사용자 확인 모델 HW-095입니다.
VMOTOR는 기능상 모터 전원 표기이며 HW-095의 실제 +5V/+12V 단자 연결은 미확정입니다.
브레드보드 + 레일은 Jetson 물리2번 5V, - 레일은 Jetson GND에 연결됩니다.
모터 드라이버 전원은 이 레일에서 공급하며 별도 외부전원 J6을 제거했습니다.
GND는 대표 핀6으로 그렸으나 실제 연결한 GND 핀 번호는 확인되지 않았습니다.
엔코더 전원과 초음파 전원은 확인되지 않아 별도 TBD 네트로 표시했습니다.
사용자 확인대로 J4 변환 회로를 제거하고 Echo를 물리36번에 직접 연결해 표시했습니다.
Echo 전압은 확인되지 않았습니다. 5V Echo 센서라면 Jetson 3.3V GPIO에 직접 입력하면 안 됩니다.
D1..D8은 LED 바의 기능상 8개 LED를 표시합니다. 실제 WCNLB8 패키지 핀 배열은 미확정입니다.
RN1은 9A331G 저항 네트워크 1개이며 내부 330Ω 저항 8개와 공통핀을 함께 그렸습니다.
공통핀은 GND, 나머지 8핀은 각 LED 캐소드에 연결했습니다.
도면에서 공통핀=1번, 분기핀=2~9번으로 가정했습니다. 실제 부품의 점 표시/저항 측정으로 확인해야 합니다.
이는 확인된 배선과 미확정 인터페이스를 구분한 문서용 초안이며 완성 PCB 제작 데이터가 아닙니다.
''',encoding='utf-8')
print(out/'SmartFan.kicad_sch')
