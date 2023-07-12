import socket
import zipfile
import paramiko
import scp
import os
import threading

#nano_ip = "192.168.86.33" #nano 4
nano_ip = ["192.168.86.116","192.168.86.116","192.168.86.116"] #nano 3
host_ip = "192.168.86.24"
nano_port = [8000,7000,6000]
host_port = [8000,7000,6000]

username = "nano"
password = "12345678"
global_model_path = [r"D:\synology\SynologyDrive\Classwork\Diondre\Senior_Design_Group_FH7\UNET\global_model_0.zip", r"D:\synology\SynologyDrive\Classwork\Diondre\Senior_Design_Group_FH7\UNET\global_model_1.zip",r"D:\synology\SynologyDrive\Classwork\Diondre\Senior_Design_Group_FH7\UNET\global_model_2.zip"]
remote_model_path = ["~/srdsg/Senior_Design_Group_FH7/UNET/jetson_0","~/srdsg/Senior_Design_Group_FH7/UNET/jetson_1","~/srdsg/Senior_Design_Group_FH7/UNET/jetson_2"]
trained_model_path = ["~/srdsg/Senior_Design_Group_FH7/UNET/jetson_0/trained_model_0.zip", "~/srdsg/Senior_Design_Group_FH7/UNET/jetson_1/trained_model_1.zip", "~/srdsg/Senior_Design_Group_FH7/UNET/jetson_2/trained_model_2.zip"]
weight_path = r"D:\synology\SynologyDrive\Classwork\Diondre\Senior_Design_Group_FH7\UNET\weights"

results = []
def fake_averaging():
    directory = weight_path
    f = open(weight_path, 'r')
    num_obj = 0
    for filename in os.listdir(directory):
        f = os.path.join(directory, filename)
        num_obj = num_obj +1
        # checking if it is a file
        if os.path.isfile(f):
            new_output = (f.readline())
            res = int(new_output[2])
            results.append(res)
            print(res)
    return sum(results)/num_obj

class DeviceHandler(threading.Thread):
    def __init__(self, nano_ip, host_ip, nano_port, host_port, trained_model_path, jetson_num, remote_model_path,global_model_path):
        threading.Thread.__init__(self)
        self.nano_ip = nano_ip
        self.host_ip =  host_ip
        self.nano_port = nano_port
        self.host_port = host_port
        self.trained_model_path = trained_model_path
        self.remote_model_path = remote_model_path
        self.jetson_num = jetson_num
        self.global_model_path = global_model_path
        return

    def send_global_model(self):
        # create a new SSH client object
        ssh = paramiko.SSHClient()

        # automatically add the host key
        ssh.set_missing_host_key_policy(paramiko.AutoAddPolicy())

        # connect to the Jetson Nano via SSH
        ssh.connect(hostname=self.nano_ip, username=username, password=password)

        # create a new SCP object
        client = scp.SCPClient(ssh.get_transport())
        # use the SCP object to transfer the file
        client.put(self.global_model_path, remote_path=self.remote_model_path)
        # close the SCP and SSH connections
        client.close()
        ssh.close()

        return

    def retrieve_global_model(self,trained_model_path): #check to see if works or not
            # create a new SSH client object
        ssh = paramiko.SSHClient()

        # automatically add the host key
        ssh.set_missing_host_key_policy(paramiko.AutoAddPolicy())

        # connect to the Jetson Nano via SSH
        ssh.connect(hostname=self.nano_ip, username=username, password=password)

        # create a new SCP object
        client = scp.SCPClient(ssh.get_transport())
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
        print(data.decode())
        # return the message as a string
        return data.decode()


    def run(self):
        print("Beginning of Thread for Jetson {}...\n".format(self.jetson_num))

        print("---START OF HOST CODE---\n")
        print("send a message to nano\n") 
        self.send_message(self.nano_ip, self.nano_port, "connected")
        print("'connected' message sent to the nano\n")
        
        print("waiting for the acknowedgement message from the nano\n")
        while True:
            if self.receive_message(self.host_ip, self.host_port) == "ACK":
                break
        print("'ACK'  message received by the host\n")


        print("zipping global model\n") 
        with zipfile.ZipFile("global_model_{}.zip".format(self.jetson_num), mode = 'w') as archive:
            archive.write(r"D:\synology\SynologyDrive\Classwork\Diondre\Senior_Design_Group_FH7\UNET\UNET_MBIMAGENET.pth")
        print("zipping of global model complete\n")

        print("sending global model to nano via ssh connection\n")
        self.send_global_model()
        print("global model has been sent to nano\n")

        print("sending a message to nano to inform the nano that the global model was sent\n")
        self.send_message(self.nano_ip,self.nano_port,"global_model_sent")
        print("global_model_sent message sent to nano\n")


        print("getting the size of the global model the host side\n")      
        LOAD_DIR = r"D:\synology\SynologyDrive\Classwork\Diondre\Senior_Design_Group_FH7\UNET"
        file_size = os.path.getsize(r"{}\global_model_{}.zip".format(LOAD_DIR,self.jetson_num))
        print("The file size of global model is {} bytes\n".format(file_size))


        print("waiting for the nano to send back the correct size of the global model\n")
        while True:
            if self.receive_message(self.host_ip,self.host_port) == "{}".format(file_size):
                break
        print("correct file size has been sent back from nano\n")


        print("removing global model on the host side\n")
        new_dir = os.path.join(LOAD_DIR, "global_model.zip")
        if os.path.exists(new_dir):
            os.remove("{}/{}".format(LOAD_DIR, "global_model.zip")) 
        print("global_model.zip has been deleted from host\n")


        print("send a message to the nano to start training\n")
        self.send_message(self.nano_ip,self.nano_port,"start_train")
        print("start_train message has been sent to nano\n")



        print("waiting for message from nano that the model has finished training\n")
        while(True):
            if(self.receive_message(self.host_ip,self.host_port) == "train_finish"):
                break
        print("train_finish message has been received by the host\n")


        print("retrieving trained model from the nano via ssh connection\n")
        self.retrieve_global_model(self.trained_model_path)
        print("host has received the trained model from nano\n")

        print("sending a message to nano to inform the nano that the trained model has been retrieved by the host laptop\n")
        self.send_message(self.nano_ip,self.nano_port,"model_sent_back")
        print("model_sent_back message has been sent to the nano\n")
    
        
        print("getting size of the trained model retrievd from nano mow on the host side\n")
        file_size = os.path.getsize(r"{}/trained_model_{}.zip".format(weight_path,self.jetson_num))
        print("The file size of the trained model is {} bytes".format(file_size))

        print("waiting for the nano to send back the correct size of the trained model\n") 
        while True:
            if self.receive_message(self.host_ip, self.host_port) == "{}".format(file_size):
                break
        print("the correct file size was sent back by nano")
        print("---END OF HOST CODE---\n")

        return

def main():

    host_thread_0 = DeviceHandler(
                                    nano_ip= nano_ip[0], 
                                    host_ip= host_ip, 
                                    nano_port= nano_port[0], 
                                    host_port=host_port[0], 
                                    trained_model_path= trained_model_path[0], 
                                    jetson_num= 0,
                                    remote_model_path = remote_model_path[0],
                                    global_model_path = global_model_path[0]
                                    )
    
    host_thread_1 = DeviceHandler(
                                nano_ip= nano_ip[1], 
                                host_ip= host_ip, 
                                nano_port= nano_port[1], 
                                host_port=host_port[1], 
                                trained_model_path= trained_model_path[1], 
                                jetson_num= 1,
                                remote_model_path= remote_model_path[1],
                                global_model_path = global_model_path[1]
                                )
    
    host_thread_2 = DeviceHandler(
                                nano_ip= nano_ip[2], 
                                host_ip= host_ip, 
                                nano_port= nano_port[2], 
                                host_port=host_port[2], 
                                trained_model_path= trained_model_path[2], 
                                jetson_num= 2,
                                remote_model_path= remote_model_path[2],
                                global_model_path = global_model_path[2]
                                )
    
    host_thread_0.start()
    host_thread_1.start()
    host_thread_2.start()

    host_thread_0.join()
    host_thread_1.join()
    host_thread_2.join()

    print("starting federated averaging")

    fake_averaging()
    
    return



if __name__ == "__main__":
    main()