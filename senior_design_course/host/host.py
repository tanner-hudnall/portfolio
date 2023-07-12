import socket
import zipfile
import paramiko
import scp
import os
import threading
import torch
import argparse

verbosity = 0

nano_port = [8000,8000,8000,8000,8000,8000]
nano_ip = ["0",
           "192.168.1.117",
           "0",
           "192.168.1.100",
           "0",
           "192.168.1.148"] #nano 0, 1, 2, 3, 4, 5

host_port = [6000,6001,6002,6003,6004,6005]
host_ip = "192.168.1.146"

username = "nano"
password = "12345678"


#host_dir = r"D:\synology\SynologyDrive\Classwork\Diondre\Senior_Design_Group_FH7\UNET"
host_dir = r"C:\Users\benhu\SynologyDrive\Classwork\Diondre\Senior_Design_Group_FH7\UNET\host" # UPDATE IF NEEDED
nano_dir = "~/senior_design"


global_model_path = rf"{host_dir}\global_model.zip"
weight_path = rf"{host_dir}\weights"


#remote_model_path = ["~/srdsg/Senior_Design_Group_FH7/UNET/jetson_0",
#                     "~/srdsg/Senior_Design_Group_FH7/UNET/jetson_1",
#                     "~/srdsg/Senior_Design_Group_FH7/UNET/jetson_2",
#                     "~/srdsg/Senior_Design_Group_FH7/UNET/jetson_3",
#                     "~/srdsg/Senior_Design_Group_FH7/UNET/jetson_4",
#                     "~/srdsg/Senior_Design_Group_FH7/UNET/jetson_5"]




#trained_model_path = ["~/srdsg/Senior_Design_Group_FH7/UNET/trained_model_0.zip",
#                      "~/srdsg/Senior_Design_Group_FH7/UNET/trained_model_1.zip",
#                      "~/srdsg/Senior_Design_Group_FH7/UNET/trained_model_2.zip",
#                      "~/srdsg/Senior_Design_Group_FH7/UNET/trained_model_3.zip",
#                      "~/srdsg/Senior_Design_Group_FH7/UNET/trained_model_4.zip",
#                      "~/srdsg/Senior_Design_Group_FH7/UNET/trained_model_5.zip"]


remote_model_path = [nano_dir, nano_dir, nano_dir, nano_dir, nano_dir, nano_dir]              

# remote_model_path = ["{}/jetson_0".format(nano_dir),
#                      "{}/jetson_1".format(nano_dir),
#                      "{}/jetson_2".format(nano_dir),
#                      "{}/jetson_3".format(nano_dir),
#                      "{}/jetson_4".format(nano_dir),
#                      "{}/jetson_5".format(nano_dir)]




trained_model_path = ["{}/trained_model_0.zip".format(nano_dir),
                      "{}/trained_model_1.zip".format(nano_dir),
                      "{}/trained_model_2.zip".format(nano_dir),
                      "{}/trained_model_3.zip".format(nano_dir),
                      "{}/trained_model_4.zip".format(nano_dir),
                      "{}/trained_model_5.zip".format(nano_dir)]

def fed_averager():
    Weights = []
    Global_Model = {}

    if(verbosity >= 2):
        print("\t[++] extracting model files from zip files and appending")
    for file in os.listdir("{}".format(weight_path)):
        zip_file_size = os.path.getsize("{}/{}".format(weight_path, file))
        if zip_file_size >= 5000000:
            with zipfile.ZipFile("{}/{}".format(weight_path, file), 'r') as zip_ref:
                zip_ref.extractall("{}/".format(weight_path))
        else:
            print(f"[!] trained model {file} is corrupted, not using for averaging")
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

    if(verbosity >= 2):
        print("\t[++] saving averaged/global model")
    torch.save(obj = Global_Model, f= rf"{host_dir}\UNET_MBIMAGENET.pth")
    

class DeviceHandler(threading.Thread):
    def __init__(self, nano_ip, host_ip, nano_port, host_port, trained_model_path, jetson_num, remote_model_path,global_model_path, verbosity):
        threading.Thread.__init__(self)
        self.nano_ip = nano_ip
        self.host_ip =  host_ip
        self.nano_port = nano_port
        self.host_port = host_port
        self.trained_model_path = trained_model_path
        self.remote_model_path = remote_model_path
        self.jetson_num = jetson_num
        self.global_model_path = global_model_path
        self.verbosity = verbosity
        return

    def send_global_model(self):
        # create a new SSH client object
        ssh = paramiko.SSHClient()

        # automatically add the host key
        ssh.set_missing_host_key_policy(paramiko.AutoAddPolicy())

        # connect to the Jetson Nano via SSH
        ssh.connect(hostname=self.nano_ip, username=username, password=password)

        # create a new SCP object
        client = scp.SCPClient(ssh.get_transport(), socket_timeout = 40)
        # use the SCP object to transfer the file
        client.put(self.global_model_path, remote_path=self.remote_model_path)
        # close the SCP and SSH connections
        client.close()
        ssh.close()

        return

    def retrieve_global_model(self,trained_model_path): 
            # create a new SSH client object
        ssh = paramiko.SSHClient()

        # automatically add the host key
        ssh.set_missing_host_key_policy(paramiko.AutoAddPolicy())

        # connect to the Jetson Nano via SSH
        ssh.connect(hostname=self.nano_ip, username=username, password=password)

        # create a new SCP object
        client = scp.SCPClient(ssh.get_transport(), socket_timeout = 40)
        # use the SCP object to transfer the file
        client.get(trained_model_path, weight_path)
        # close the SCP and SSH connections
        client.close()
        ssh.close()

    def send_message(self,ip_address, port, message):
        # create a socket object
        sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        
        # connect to the specified IP address and port
        sock.connect((ip_address, port))
        
        # send the message
        sock.sendall(message.encode())
        
        # close the socket
        sock.close()

    def receive_message(self, ip_address, port):
        # create a socket object
        sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        
        # bind the socket to the specified IP address and port
        sock.bind((ip_address, port))

        # listen for incoming connections
        sock.listen(1)
        
        # accept the first incoming connection
        conn, addr = sock.accept()
        
        # receive the message
        data = conn.recv(1024)
        
        # close the connection and socket
        conn.close()
        sock.close()
        
        # return the message as a string
        return data.decode()


    def run(self):
        if(self.verbosity >= 2):
            print("\t[++] [{}] Beginning of Thread for Jetson {}...\n".format(self.jetson_num,self.jetson_num))

        #print("send a message to nano\n") 
        self.send_message(self.nano_ip, self.nano_port, "connected")
        #print("'connected' message sent to the nano\n")
        
        if(self.verbosity >= 2):
            print("\t[++] [{}] waiting for the connection acknowedgement message from the nano\n".format(self.jetson_num))
        while True:
            if self.receive_message(self.host_ip, self.host_port) == "ACK":
                break
        #print("'ACK'  message received by the host\n")
        if(self.verbosity >= 2):
            print("\t[++] [{}] connected to jetson {}\n".format(self.jetson_num, self.jetson_num))

        #print("sending global model to nano via ssh connection\n")
        self.send_global_model()
        if(self.verbosity >= 2):
            print("\t[++] [{}] global model has been sent to nano\n".format(self.jetson_num))

        #print("sending a message to nano to inform the nano that the global model was sent\n")
        self.send_message(self.nano_ip,self.nano_port,"global_model_sent")
        #print("global_model_sent message sent to nano\n")


        #print("getting the size of the global model the host side\n")      
        file_size = os.path.getsize(r"{}\global_model.zip".format(host_dir))
        #print("The file size of global model is {} bytes\n".format(file_size))

        #print("waiting for the nano to send back the correct size of the global model\n")
        while True:
            if self.receive_message(self.host_ip,self.host_port) == "{}".format(file_size):
                break
        
        #print("correct file size has been sent back from nano\n")
        if(self.verbosity >= 2):
            print("\t[++] [{}] file transferred sucessfully\n".format(self.jetson_num))

        #print("send a message to the nano to start training\n")
        self.send_message(self.nano_ip,self.nano_port,"start_train")
        #print("start_train message has been sent to nano\n")

        if(self.verbosity >= 2):
            print("\t[++] [{}] training started -- waiting for message from nano that the model has finished training\n".format(self.jetson_num))
        while(True):
            if(self.receive_message(self.host_ip,self.host_port) == "train_finish"):
                break
        #print("train_finish message has been received by the host\n")


        #print("retrieving trained model from the nano via ssh connection\n")
        self.retrieve_global_model(self.trained_model_path)
        if(self.verbosity >= 2):
            print("\t[++] [{}] host has received the trained model from nano\n".format(self.jetson_num))

        #print("sending a message to nano to inform the nano that the trained model has been retrieved by the host laptop\n")
        self.send_message(self.nano_ip,self.nano_port,"model_sent_back")
        #print("model_sent_back message has been sent to the nano\n")
    
        
        #print("getting size of the trained model retrievd from nano mow on the host side\n")
        file_size = os.path.getsize(r"{}/trained_model_{}.zip".format(weight_path,self.jetson_num))
        #print("The file size of the trained model is {} bytes".format(file_size))

        #print("waiting for the nano to send back the correct size of the trained model\n") 
        while True:
            if self.receive_message(self.host_ip, self.host_port) == "{}".format(file_size):
                break
        #print("the correct file size was sent back by nano")
        if(self.verbosity >= 2):
            print(f"\t[++] [{self.jetson_num}] file transferred sucessfully\n")
            print(f"\t[++] [{self.jetson_num}] thread finished for jetson{self.jetson_num}\n")

        return

def arg_parser():
    # python host.py -host hostip -ip1 ip1 -ip3 ip3 -ip4 ip4 -ip5 ip5 -verbosity #
    parser = argparse.ArgumentParser(description='host.py, host code for federated learning, to connect to jetson nanos')
    parser.add_argument('-host', '--host', type=str, required=True, help='host ip (this device)')
    parser.add_argument('-ip1', '--ip1', type=str, default="0", help='jetson #1 ip')
    parser.add_argument('-ip2', '--ip2', type=str, default="0", help='jetson #2 ip')
    parser.add_argument('-ip3', '--ip3', type=str, default="0", help='jetson #3 ip')
    parser.add_argument('-ip4', '--ip4', type=str, default="0", help='jetson #4 ip')
    parser.add_argument('-ip5', '--ip5', type=str, default="0", help='jetson #5 ip')
    parser.add_argument('-v', '--verbosity', type=int, default=0, help='verbosity: 0 - only error msgs, 1 - main msgs, 2 - detailed msgs (default 0)')
    parser.add_argument('-l', '--loops', type=int, default=1, help='number of federated learning loops (default 1)')

    return parser.parse_args()


def main():

    arguments = arg_parser()
    # error statements for no ips --- DONE
    host_ip = arguments.host
    nano_ip[1] = arguments.ip1 
    nano_ip[2] = arguments.ip2
    nano_ip[3] = arguments.ip3
    nano_ip[4] = arguments.ip4
    nano_ip[5] = arguments.ip5
    verbosity = arguments.verbosity
    loops = arguments.loops
   

    print(f"host ip: {host_ip}")
    print(f"nano 1: {nano_ip[1]}")
    print(f"nano 2: {nano_ip[2]}")
    print(f"nano 3: {nano_ip[3]}")
    print(f"nano 4: {nano_ip[4]}")
    print(f"nano 5: {nano_ip[5]}")
    print(f"training loops: {loops}")
    print(f"verbosity level: {verbosity}")
    



    #return
    
    print("\n---RUNNING HOST.PY---\n")
    if(verbosity >= 1):
        print("[+] initalizing workspace")
    # deletes zip files and pth files in the host dir
    if os.path.exists(global_model_path):
        os.remove(global_model_path) 
        if(verbosity >= 2):
            print("\t[++] global_model.zip has been deleted from host\n")
    if os.path.exists(rf"{host_dir}\UNET_MBIMAGENET.pth"):
        os.remove(rf"{host_dir}\UNET_MBIMAGENET.pth")
        if(verbosity >= 2):
            print("\t[++] model file has been deleted from host\n")

    for file in os.listdir("{}".format(weight_path)):
        os.remove("{}\{}".format(weight_path, file))
    if(verbosity >= 2):
        print("\t[++] weights folder emptied\n")

    pretrained = rf"{host_dir}\pretrained_model\UNET_MBIMAGENET.pth"
    os.system(f'copy {pretrained} {host_dir}')
    if(verbosity >= 2):
            print("\t[++] pretrained model copied into working dir\n")




    # TODO make sure to delete anything in the weights folder 

    # Federated learning loop begins here
    for i in range(loops):
        host_thread_1 = DeviceHandler(
                                    nano_ip= nano_ip[1], 
                                    host_ip= host_ip, 
                                    nano_port= nano_port[1], 
                                    host_port=host_port[1], 
                                    trained_model_path= trained_model_path[1], 
                                    jetson_num= 1,
                                    remote_model_path = remote_model_path[1],
                                    global_model_path = global_model_path,
                                    verbosity=verbosity
                                    )

        host_thread_2 = DeviceHandler(
                                    nano_ip= nano_ip[2], 
                                    host_ip= host_ip, 
                                    nano_port= nano_port[2], 
                                    host_port=host_port[2], 
                                    trained_model_path= trained_model_path[2], 
                                    jetson_num= 2,
                                    remote_model_path = remote_model_path[2],
                                    global_model_path = global_model_path,
                                    verbosity=verbosity
                                    )

        host_thread_3 = DeviceHandler(
                                    nano_ip= nano_ip[3], 
                                    host_ip= host_ip, 
                                    nano_port= nano_port[3], 
                                    host_port=host_port[3], 
                                    trained_model_path= trained_model_path[3], 
                                    jetson_num= 3,
                                    remote_model_path = remote_model_path[3],
                                    global_model_path = global_model_path,
                                    verbosity=verbosity
                                    )
    
        host_thread_4 = DeviceHandler(
                                    nano_ip= nano_ip[4], 
                                    host_ip= host_ip, 
                                    nano_port= nano_port[4], 
                                    host_port=host_port[4], 
                                    trained_model_path= trained_model_path[4], 
                                    jetson_num= 4,
                                    remote_model_path= remote_model_path[4],
                                    global_model_path = global_model_path,
                                    verbosity=verbosity
                                    )

        host_thread_5 = DeviceHandler(
                                    nano_ip= nano_ip[5], 
                                    host_ip= host_ip, 
                                    nano_port= nano_port[5], 
                                    host_port=host_port[5], 
                                    trained_model_path= trained_model_path[5], 
                                    jetson_num= 5,
                                    remote_model_path = remote_model_path[5],
                                    global_model_path = global_model_path,
                                    verbosity=verbosity
                                    )

        if(verbosity >= 1):
            print("[+] federated learning loop #{}\n".format(i+1))
        
        if(verbosity >= 2):
            print("\t[++] zipping global model\n") 
        with zipfile.ZipFile("global_model.zip", mode = 'w') as archive:
            archive.write(r"UNET_MBIMAGENET.pth")
        #if(verbosity >= 2):
        #    print("\t[++] zipping of global model complete\n")

        if(verbosity >= 2):
            print("\t[++] starting threads for each nano\n")
        if nano_ip[1] != "0":
            host_thread_1.start()
        if nano_ip[2] != "0":
            host_thread_2.start()
        if nano_ip[3] != "0":
            host_thread_3.start()
        if nano_ip[4] != "0":
            host_thread_4.start()
        if nano_ip[5] != "0":
            host_thread_5.start()
        

        if nano_ip[1] != "0":
            host_thread_1.join()
        if nano_ip[2] != "0":
            host_thread_2.join()
        if nano_ip[3] != "0":
            host_thread_3.join()
        if nano_ip[4] != "0":
            host_thread_4.join()
        if nano_ip[5] != "0":
            host_thread_5.join()
        
        if(verbosity >= 2):
            print("\t[++] removing global model on the host side\n")
        if os.path.exists(global_model_path):
            os.remove(global_model_path) 
        #if(verbosity >= 2):
        #    print("\t[++] global_model.zip has been deleted from host\n")
        
        if(verbosity >= 1):
            print("[+] averaging models")
        fed_averager()
        if(verbosity >= 2):
            print("\t[++] federated averaging complete")
        # end of federation loop, should repeat as needed

    print("federated learning finished")

    return



if __name__ == "__main__":
    main()