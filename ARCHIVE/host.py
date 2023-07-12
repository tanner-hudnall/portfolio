import paramiko
import scp
import subprocess
import time
import socket
import os
import torch
import zipfile

# TODO move these into a python file only for paths and variables
global_model_path = r"D:\synology\SynologyDrive\Classwork\Diondre\Senior_Design_Group_FH7\UNET\global_model.zip"  
remote_model_path = "~/srdsg/Senior_Design_Group_FH7/UNET/"
remote_model_file = "~/srdsg/Senior_Design_Group_FH7/UNET/trained_model.zip"
weight_path = r"D:\synology\SynologyDrive\Classwork\Diondre\Senior_Design_Group_FH7\UNET\weights"

nano_ip = "192.168.86.33"
username = "nano"
password = "12345678"

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
    

def communication_rec():
    # create a socket object
    with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as s:
    # bind the socket to a specific network interface and port number
        s.bind(("0.0.0.0", 22))
        # listen for incoming connections
        s.listen()
        print('waiting for nano #1 message at {}:{}...'.format("0.0.0.0", 22))

        # accept a nano connection
        conn, addr = s.accept()
        with conn:
            print('Connected by', addr)

            # receive data from the nano
            data = conn.recv(1024)
            print('Received data:', data.decode())
            received_msg = data.decode()
    s.close
    return received_msg


def communication_send(message_to_send):
    # create a socket object
    with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as s:
        # connect to the server
        s.connect((nano_ip, 1024))
        print('Connected to', nano_ip)

        # send data to the server
        message = message_to_send
        s.sendall(message.encode())
    s.close
    return


def send_global_model():

    # create a new SSH client object
    ssh = paramiko.SSHClient()

    # automatically add the host key
    ssh.set_missing_host_key_policy(paramiko.AutoAddPolicy())

    # connect to the Jetson Nano via SSH
    ssh.connect(hostname=nano_ip, username=username, password=password)

    # create a new SCP object
    client = scp.SCPClient(ssh.get_transport())
    # use the SCP object to transfer the file
    client.put(global_model_path, remote_path=remote_model_path)
    # close the SCP and SSH connections
    client.close()
    ssh.close()

    return


def retrieve_global_model(): #check to see if works or not
        # create a new SSH client object
    ssh = paramiko.SSHClient()

    # automatically add the host key
    ssh.set_missing_host_key_policy(paramiko.AutoAddPolicy())

    # connect to the Jetson Nano via SSH
    ssh.connect(hostname=nano_ip, username=username, password=password)

    # create a new SCP object
    client = scp.SCPClient(ssh.get_transport())
    # use the SCP object to transfer the file
    client.get(remote_model_file, weight_path)
    # close the SCP and SSH connections
    client.close()
    ssh.close()

    return


def file_transfer_check(file):
    local_size = os.path.getsize(file)
    while(True):
        if communication_rec() == "{}".format(local_size):
            print("file transferred sucessfully")
            break

    return

LOAD_DIR = "."

# TODO initialze all folders etc

# TODO resend file incase of send failure, time.sleep


def main():
    print("running host.py")
    # connection acknowledgement
    while(True):
        if(communication_rec() == "connected"):
            break


    # loop for multiple federated learning cycles
    for i in range(2):
        print("federated learning loop #", i)

        with zipfile.ZipFile("global_model.zip", mode = 'w') as archive:
            archive.write(r"UNET_MBIMAGENET.pth")
        
        
        # send global models
        print("Sending global models to the nanos...")
        send_global_model()
        communication_send("model_sent")
        print("global model sent to all nanos!")

        
        # check file sent correctly
        print("checking file sent sucessfully...")
        file_transfer_check(global_model_path)

        
        os.remove("{}/{}".format(LOAD_DIR, "global_model.zip"))

        # send message to nano to start training models
        print("starting to train models...")
        communication_send("start_train")


        # wait for training to be finished
        print("waiting for \"train_finish\" message...")
        while(True):
            if(communication_rec() == "train_finish"):
                break
        print("training complete!")


        # pull trained models from nanos
        print("receiving trained models...")
        retrieve_global_model()
        communication_send("model_sent_back")
        print("trained models downloaded!")


        # check file sent correctly
        print("checking file sent sucessfully...")
        
        file_transfer_check("{}/{}".format(weight_path, "trained_model.zip"))

        # aggregate the models together
        print("beginning aggregation of models...")
        fed_averager()
        print("aggregation complete")
    return

if __name__ == "__main__":
    main()



