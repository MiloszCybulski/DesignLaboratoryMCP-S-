from tkinter import *
from tkinter.scrolledtext import ScrolledText
import serial

s = serial.Serial('/dev/ttyUSB0', 9600, timeout=1)
root = Tk()
root.title("UART-data visualization")

frame = Frame(root)
frame.pack(padx=10, pady=10)

log = ScrolledText(
    frame,
    width=60,
    height=15,
    font=("Consolas", 11)
)
log.pack(fill="both", expand=True)

led_canvas = Canvas(root, width=40, height=40, highlightthickness=0)
led_canvas.pack(padx=10, pady=10)
led_id = led_canvas.create_oval(5, 5, 35, 35, fill="gray20", outline="gray50")
led_on = False 
led_label = Label(root, text="LED", font=("Arial", 12))
led_label.pack(pady=2)

buttons = Frame(root)
buttons.pack(side="top", fill="x", padx=10, pady=5)

## LED ### 
def set_led(state: bool) -> None:
    """Ustawia kolor diody."""
    color = "lime green" if state else "gray20"
    led_canvas.itemconfig(led_id, fill=color)

### Tx ### 
def send_uart(cmd):
    s.write((cmd + "\n").encode("utf-8"))
    log.insert("end", f">>> {cmd}\n")
    log.see("end")

def call_temp():
    send_uart("t")

def call_psi():
    send_uart("p")

def call_led_on(state: bool) -> None:
    send_uart("d")
    global led_on
    led_on = state
    set_led(led_on)

def call_led_off(state: bool) -> None:
    send_uart("a")
    global led_on
    led_on = state
    set_led(led_on)

def call_reset():
    send_uart("r")


### BUTTONS ### 
btn_temp = Button(buttons, text="°C", command=call_temp)
btn_temp.pack(side="left", padx=5)

btn_pressure = Button(buttons, text="PSI", command=call_psi)
btn_pressure.pack(side="left", padx=5)

btn_led_on = Button(buttons, text="LED ON", command= lambda: call_led_on(True))
btn_led_on.pack(side="left", padx=5)

btn_led_off = Button(buttons, text="LED OFF", command= lambda: call_led_off(False))
btn_led_off.pack(side="left", padx=5)

btn_reset = Button(buttons, text="RESET", command=call_reset)
btn_reset.pack(side="left", padx=5)

### Rx ### 
def read_uart():
    while s.in_waiting:
        line = s.readline().decode("utf-8", errors="ignore")
        line = line.replace("\r\n", "\n").replace("\r", "\n")
        if line:
            log.insert("end", line)
            log.see("end")  

    root.after(100, read_uart)


read_uart()
root.mainloop()














# e = Entry(root, width=20)
# e.pack()

# def myClick():
#     myLabel = Label(root, text="Hello Elo")
#     myLabel.pack()

# def myTempClick():
#     tempLabel = Label(root, text="BMP")
#     tempLabel.pack()

# myButton = Button(root, text="Click Me!", command=myClick, fg="red", bg="#000000")
# myButton.grid(row=1, column=0)



# tempButton = Button(root, text="Update Temp", command=myTempClick, fg="red", bg="#000000")
# myButton.grid(row=2, column=1)

# def button_add():
#     return

# tempButton     =  Button(root, text=" Temperature", width=10, pady=20, command=button_add)
# pressureButton =  Button(root, text="  Pressure  ", width=10, pady=20, command=button_add)
# ledOnButton    =  Button(root, text="   LED ON   ", width=10, pady=20, command=button_add)
# led0ffButton   =  Button(root, text="   LED OFF  ", width=10, pady=20, command=button_add)
# resetButton    =  Button(root, text="    RESET   ", width=10, pady=20, command=button_add)

# tempButton.grid(row=0, column=0)
# pressureButton.grid(row=1, column=0)
# ledOnButton.grid(row=2, column=0)
# led0ffButton.grid(row=3, column=0)
# resetButton.grid(row=4, column=0)

