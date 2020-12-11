# Program that communicates with the MSP432-P401R board that
# has the MonitorMan c-program installed. It has the ability 
# to check the current state of the board and also manupulate
# the state using a GUI.
import serial
import tkinter as tk
import threading

#initialzing the serial port
ser = serial.Serial('COM4')

# Writes the next state command
def nextState():
    ser.write(b"N")

# Writes the previous state command
def prevState():
    ser.write(b"P")

# Gets the current state
def getCurrentState():
    ser.write(b"C")
    return int.from_bytes(ser.read(1), byteorder="big")

# Stream that constantly keeps track of the current state
# of the board and updates the GUI if there is a change
def currentStateStream():
    currentState = getCurrentState()
    lbl_value["text"] = str(currentState + 1)
    while True:
        bytesToRead = ser.inWaiting()
        state = int.from_bytes(ser.read(1), byteorder="big")
        if (state != currentState):
            currentState = state
            lbl_value["text"] = str(state + 1)


# Setting up the window
window = tk.Tk()
window.rowconfigure(0, minsize=50, weight=1)
window.columnconfigure([0, 1, 2], minsize=50, weight=1)

# Setting up previous button
btn_prev = tk.Button(master=window, text="<<< Previous State", command=prevState)
btn_prev.grid(row=0, column=0, sticky="nsew")

# Setting up current state label
lbl_value = tk.Label(master=window, text="10")
lbl_value.grid(row=0, column=1)

# Setting up next button
btn_next = tk.Button(master=window, text="Next State >>>", command=nextState)
btn_next.grid(row=0, column=2, sticky="nsew")

# Initiating seperate thread for the currentStateStream() function
threading.Thread(target=currentStateStream, args=(), daemon=True).start()

# Main loop
window.mainloop()

ser.close()