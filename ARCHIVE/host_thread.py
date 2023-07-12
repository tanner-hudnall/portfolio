import paramiko
import scp
import subprocess
import time
import paths
import socket
import os
import torch
import zipfile
from threading import Thread


# TODO move these into a python file only for paths and variables
#Note - create a file caled paths.py LOCALLY and it should work (create it within same directory DO NOT UPLOAD TO GIT)
local_p = paths.global_model_path
global_model_path = paths.global_model_path
remote_model_path = "~/srdsg/Senior_Design_Group_FH7/UNET/"
remote_model_file = "~/srdsg/Senior_Design_Group_FH7/UNET/trained_model.zip"
weight_path = paths.weight_path

nano_ip = ["172.20.10.6"]
username = ["nano"]
password = ["12345678"]
port = [(1000,1024), (23,1025), (24,1026)]

class DeviceHandler(Thread):
    def __init__(self, nano_ip, username, password, port):
        # execute the base constructor
        Thread.__init__(self)
        self.nano_ip = nano_ip
        self.username = username
        self.password = password
        self.port = port[0]
        self.port1 = port[1]

    def communication_rec(self):
        # create a socket object
        with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as s:
        # bind the socket to a specific network interface and port number
            s.bind(("0.0.0.0", self.port))
            # listen for incoming connections
            s.listen()
            print('waiting for nano #1 message at {}:{}...'.format("0.0.0.0", self.port))

            # accept a nano connection
            conn, addr = s.accept()
            with conn:
                print('Connected by', addr)

                # receive data from the nano
                data = conn.recv(self.port1)
                print('Received data:', data.decode())
                received_msg = data.decode()
        s.close
        return received_msg


    def communication_send(self, message_to_send):
        # create a socket object
        with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as s:
            # connect to the server
            s.connect((nano_ip, self.port1))
            print('Connected to', nano_ip)

            # send data to the server
            message = message_to_send
            s.sendall(message.encode())
        s.close
        return


    def send_global_model(self):

        # create a new SSH client object
        ssh = paramiko.SSHClient()

        # automatically add the host key
        ssh.set_missing_host_key_policy(paramiko.AutoAddPolicy())

        # connect to the Jetson Nano via SSH
        ssh.connect(hostname=self.nano_ip, username=self.username, password=self.password, port =self.port)

        # create a new SCP object
        client = scp.SCPClient(ssh.get_transport())
        # use the SCP object to transfer the file
        client.put(global_model_path, remote_path=remote_model_path)
        # close the SCP and SSH connections
        client.close()
        ssh.close()

        return


    def retrieve_global_model(self): #check to see if works or not
            # create a new SSH client object
        ssh = paramiko.SSHClient()

        # automatically add the host key
        ssh.set_missing_host_key_policy(paramiko.AutoAddPolicy())

        # connect to the Jetson Nano via SSH
        ssh.connect(hostname=self.nano_ip, username=self.username, password=self.password, port=self.port)

        # create a new SCP object
        client = scp.SCPClient(ssh.get_transport())
        # use the SCP object to transfer the file
        client.get(remote_model_file, weight_path)
        # close the SCP and SSH connections
        client.close()
        ssh.close()

        return


    def file_transfer_check(self, file):
        local_size = os.path.getsize(file)
        print(local_size)
        while(True):
            if communication_rec() == "{}".format(local_size):
                print("file transferred sucessfully")
                break

        return

    LOAD_DIR = "."

    def run(self):
        print("running host.py")
        # connection acknowledgement
        while(True):
            if(self.communication_rec() == "connected"):
                break


        # loop for multiple federated learning cycles
        for i in range(2):
            print("federated learning loop #", i)

            with zipfile.ZipFile("global_model.zip", mode = 'w') as archive:
                archive.write(r"UNET_MBIMAGENET.pth")
            
            
            # send global models
            print("Sending global models to the nanos...")
            self.send_global_model()
            self.communication_send("model_sent")
            print("global model sent to all nanos!")

            
            # check file sent correctly
            print("checking file sent sucessfully...")
            self.file_transfer_check(global_model_path)

            
            os.remove("{}/{}".format(LOAD_DIR, "global_model.zip"))

            # send message to nano to start training models
            print("starting to train models...")
            self.communication_send("start_train")


            # wait for training to be finished
            print("waiting for \"train_finish\" message...")
            while(True):
                if(self.communication_rec() == "train_finish"):
                    break
            print("training complete!")


            # pull trained models from nanos
            print("receiving trained models...")
            self.retrieve_global_model()
            self.communication_send("model_sent_back")
            print("trained models downloaded!")


            # check file sent correctly
            print("checking file sent sucessfully...")
            
            self.file_transfer_check("{}/{}".format(weight_path, "trained_model.zip"))
        return

def fed_averager():
        Weights = []
        Global_Model = {}

        for file in os.listdir("{}".format(weight_path)):
            with zipfile.ZipFile("{}/{}".format(weight_path, file), 'r') as zip_ref:
                zip_ref.extractall("{}/".format(weight_path))
            os.remove("{}\{}".format(weight_path, file))

        for file in os.listdir("{}".format(weight_path)):
            Weights.append(torch.load("{}/{}".format(weight_path, file), map_location=torch.device('cpu')))
            os.remove("{}/{}".format(weight_path, file))

        for i in range(Weights.__len__()):
            for key in Weights[0]:
                temp = Weights[i][key]
                if key in Global_Model:
                    Global_Model[key] += temp
                else:
                    Global_Model[key] = temp

        for key in Global_Model:
            Global_Model[key] = Global_Model[key] / Weights.__len__()

        torch.save(obj = Global_Model, f= global_model_path)

def main():
    num_devices = 3
    num_cycles = 4

    # start fed loop here
    for j in range(num_cycles):
        print("federated averaging loop {}".format(j))
        for i in range(num_devices):
            devices = DeviceHandler(nano_ip[0], username[0] , password[0], port[i])
            print("Declare thread #1")
            devices.start()
            devices.join()

            # aggregate the models together
            print("beginning aggregation of models...")
            fed_averager()
            print("aggregation complete")
            #repeat loop

    

if __name__ == "__main__":
    main()

