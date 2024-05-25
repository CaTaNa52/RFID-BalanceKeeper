import tkinter as tk
from tkinter import ttk, messagebox, filedialog
import serial
import serial.tools.list_ports
import csv
from datetime import datetime
import os
import threading
import plotly.graph_objects as go
from plotly.subplots import make_subplots
from ttkthemes import ThemedStyle
import sv_ttk
import plotly.io as pio

class SerialLoggerApp:
    def __init__(self, root):
        self.root = root
        self.root.title("RFID Balance Keeper Logger")

        self.portVar = tk.StringVar()
        self.baudrateVar = tk.StringVar(value='9600')
        self.serialInst = None
        self.logging = False
        self.save_path = tk.StringVar()

        # GUI Layout
        self.create_widgets()

        # Plotly Figure
        self.fig = make_subplots(rows=1, cols=1)
        self.fig.update_layout(title="Echtzeit Daten", xaxis_title="Uhrzeit", yaxis_title="Einnahmen in Euro")
        self.fig.update_xaxes(title_font=dict(size=18, family='Arial', color='black', weight='bold'))
        self.fig.update_yaxes(title_font=dict(size=18, family='Arial', color='black', weight='bold'))

        # Data storage
        self.timestamps = []
        self.data = []

    def create_widgets(self):

        sv_ttk.set_theme("dark")

        # Frame for COM Port and Baudrate selection
        frame = ttk.Frame(self.root)
        frame.pack(padx=10, pady=5)

        # Dropdown for COM-Port Auswahl
        ttk.Label(frame, text="COM Port:").grid(column=0, row=0, padx=5, pady=5, sticky="w")
        self.port_combobox = ttk.Combobox(frame, textvariable=self.portVar, state="readonly")
        self.port_combobox['values'] = self.get_ports()
        self.port_combobox.grid(column=1, row=0, padx=5, pady=5)


        # Dropdown for Baudrate Auswahl
        ttk.Label(frame, text="Baudrate:").grid(column=0, row=2, padx=5, pady=5, sticky="w")
        baudrates = ['9600', '19200', '38400', '57600', '115200']
        self.baudrate_combobox = ttk.Combobox(frame, textvariable=self.baudrateVar, values=baudrates)
        self.baudrate_combobox.grid(column=1, row=2, padx=5, pady=5)

        # Button to select save path
        self.select_path_button = ttk.Button(frame, text="Speicherpfad wählen", command=self.select_save_path)
        self.select_path_button.grid(column=0, row=3, columnspan=2, padx=5, pady=10)

        # Start/Stop Buttons
        self.start_button = ttk.Button(frame, text="Start Logging", command=self.start_logging)
        self.start_button.grid(column=0, row=4, columnspan=2, padx=5, pady=5)

        self.stop_button = ttk.Button(frame, text="Stop Logging", command=self.stop_logging)
        self.stop_button.grid(column=0, row=5, columnspan=2, padx=5, pady=5)
        self.stop_button['state'] = 'disabled'

        # Text widget to display logged values
        self.log_display = tk.Text(self.root, height=10, width=50)
        self.log_display.pack(padx=10, pady=10)

        # Button to show plot
        self.plot_button = ttk.Button(self.root, text="Show Plot", command=self.show_plot)
        self.plot_button.pack(padx=10, pady=5)

    def get_ports(self):
        ports = list(serial.tools.list_ports.comports())
        return [f"{port.device} ({port.description})" for port in ports]

    def select_save_path(self):
        self.save_path.set(filedialog.askdirectory())

    def start_logging(self):
        if not self.portVar.get():
            messagebox.showwarning("Warnung", "Bitte einen COM-Port auswählen.")
            return

        if not self.save_path.get():
            messagebox.showwarning("Warnung", "Bitte wählen Sie einen Speicherpfad aus.")
            return

        self.logging = True
        self.start_button['state'] = 'disabled'
        self.stop_button['state'] = 'normal'

        self.serialInst = serial.Serial(self.portVar.get().split()[0], int(self.baudrateVar.get()))
        self.log_thread = threading.Thread(target=self.log_data)
        self.log_thread.start()

    def stop_logging(self):
        self.logging = False
        self.start_button['state'] = 'normal'
        self.stop_button['state'] = 'disabled'
        if self.serialInst:
            self.serialInst.close()

        # Save data to CSV
        self.save_to_csv()

    def log_data(self):
        while self.logging:
            if self.serialInst and self.serialInst.in_waiting > 0:
                packet = self.serialInst.readline()
                data = packet.decode("utf-8").rstrip('\n')
                timestamp = datetime.now().strftime('%Y-%m-%d %H-%M-%S')
                log_entry = f"{timestamp} - {data}"
                print(log_entry)

                # Update log display
                self.log_display.insert(tk.END, log_entry + '\n')
                self.log_display.see(tk.END)

                # Update plot data
                self.timestamps.append(timestamp)
                self.data.append(float(data))

    def save_to_csv(self):
        filename = os.path.join(self.save_path.get(), f"einnahmen_{datetime.now().strftime('%Y-%m-%d_%H-%M-%S')}.csv")
        with open(filename, 'w', newline='') as file:
            writer = csv.writer(file)
            writer.writerow(["Timestamp", "Einnahmen"])
            writer.writerows(zip(self.timestamps, self.data))

        # Save plot as HTML
        plot_filename = os.path.join(self.save_path.get(), f"einnahmen_plot_{datetime.now().strftime('%Y-%m-%d_%H-%M-%S')}.html")
        pio.write_html(self.fig, plot_filename)

    def show_plot(self):
        if not self.timestamps or not self.data:
            messagebox.showinfo("Information", "Keine Daten zum Plotten verfügbar.")
            return

        self.fig.data = []  # Clear existing data
        self.fig.add_trace(go.Scatter(x=self.timestamps, y=self.data, mode='lines', name='Einnahmen'))
        self.fig.show()

if __name__ == "__main__":
    root = tk.Tk()
    app = SerialLoggerApp(root)
    root.mainloop()
