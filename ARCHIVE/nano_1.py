import socket
import zipfile
import os
from UNETv3_TrainFunc import TrainingLoop 

#nano_ip = "192.168.86.33" #nano 4
nano_ip = "192.168.86.116" #nano 3
host_ip = "192.168.86.24"
nano_port = 7000
host_port = 7000

def send_message(ip_address, port, message):
    # create a socket object
    sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    
    # connect to the specified IP address and port
    sock.connect((ip_address, port))
    
    # send the message
    sock.sendall(message.encode())
    
    # close the socket
    sock.close()

def receive_message(ip_address, port):
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
    print("{}", format(data.decode))
    # return the message as a string
    return data.decode()

def main():
    print("---START OF NANO CODE---\n")

 
    print("creating a folder for jetson\n")
    folder_name = "jetson_1" #EDIT CONTAINS JETSON NUMBER
    current_dir = os.getcwd()
    new_dir = os.path.join(current_dir, folder_name)
    if not os.path.exists(new_dir):
        os.makedirs(new_dir)
        print(f"Directory {new_dir} created successfully\n")
    else:
        print(f"Directory {new_dir} already exists\n")


    print("waiting for message from host laptop...\n")
    while True:
        if receive_message(nano_ip,nano_port) == "connected":
            break
    print("recieved 'connected' message from host\n")   


    print("sending host an acknowledgement message\n")
    send_message(host_ip,host_port,"ACK")
    print("'ACK' message sent")

    
    print("waiting for a message from the host laptop informing the nano that global model was sent...\n")
    while True:
        if receive_message(nano_ip, nano_port) == "global_model_sent":
            break
    print("received global 'global_model_sent'\n")


    print("getting the size of the global model on the nano side\n")        
    LOAD_DIR = "/home/nano/srdsg/Senior_Design_Group_FH7/UNET/jetson_1"  #EDIT CONTAINS JETSON NUMBER
    file_size = os.path.getsize(r"{}/global_model.zip".format(LOAD_DIR)) #EDIT CONTAINS JETSON NUMBER
    print("file size on the nano # side is {} bytes\n".format(file_size))

    print("sending a number representing the size of the global model to host\n")
    send_message(host_ip, host_port,"{}".format(file_size))
    print("file size sent to host\n")


    print("waiting for message from host to begin training...\n")
    while(True):
        if(receive_message(nano_ip,nano_port) == "start_train"):
            break
    print("'start_train' message has been sent to host\n")


    print("extract the global model from its zip file\n")
    with zipfile.ZipFile("{}/{}".format(LOAD_DIR, "global_model.zip"), 'r') as zip_ref: #EDIT CONTAINS JETSON NUMBER
        zip_ref.extractall("{}/".format(LOAD_DIR))
        os.remove("{}/{}".format(LOAD_DIR, "global_model.zip")) #EDIT CONTAINS JETSON NUMBER
    print("global_model_1.zip has been extracted for training loop and subsequently deleted\n") #EDIT CONTAINS JETSON NUMBER

    print("setting name of trained model file\n")
    trained_model_file_name = "nano_1_trained_model.pth" #EDIT CONTAINS JETSON NUMBER
    print("name of trained model is {}\n".format(trained_model_file_name))

    print("entering jetson folder")
    folder_name = "jetson_1"  #EDIT CONTAINS JETSON NUMBER
    current_dir = os.getcwd()
    folder_path = os.path.join(current_dir, folder_name)

    if os.path.exists(folder_path):
        os.chdir(folder_path)
        print(f"Changed working directory to {folder_path}")
    else:
        print(f"Directory {folder_path} does not exist")

    print("starting to training model on nano\n")
    model = TrainingLoop(2, 1, .0001, trained_model_file_name, jetson_num=1)
    print("training of model complete\n")

    print("zipping the trained model on nano\n")
    with zipfile.ZipFile("trained_model_1.zip", mode = 'w') as archive: #EDIT CONTAINS JETSON NUMBER
        archive.write(trained_model_file_name)
    print("zipping of trained model complete\n")


    print("sending a message to host that the model has finished training\n")
    send_message(host_ip,host_port,"train_finish")
    print("'train_finish' message sent to host\n")


    print("waiting for a meesage from the host that the trained model was retrieved by the host\n")
    while(True):
        if(receive_message(nano_ip,nano_port) == "model_sent_back"):
            break
    print("'model_sent_back' message has been received by nano\n")


    print("getting size of the trained model on the nano side\n")
    file_size = os.path.getsize(r"{}/trained_model_1.zip".format(LOAD_DIR)) #EDIT CONTAINS JETSON NUMBER
    print("file size of the trained model is {} bytes\n".format(file_size))

    print("sending a number to the host laptop reprensenting the size of the trained model on the nano side\n")
    send_message(host_ip,host_port,"{}".format(file_size))
    print("file size of trained model has been sent to host")

    print("removing trained model from the nano side\n")
    os.remove("{}/{}".format(LOAD_DIR, trained_model_file_name))
    os.remove("{}/{}".format(LOAD_DIR, "trained_model_1.zip")) #EDIT CONTAINS JETSON NUMBER

    print("exiting jetson folder")
    current_dir = os.getcwd()
    if os.path.basename(current_dir) == "jetson_1": #EDIT CONTAINS JETSON NUMBER
        os.chdir('..')
        print(f"Exited {current_dir}, current working directory is now {os.getcwd()}")
    else:
        print(f"Error: current directory is not jetson_1")  #EDIT CONTAINS JETSON NUMBER 

    print("---END OF NANO CODE---\n")

    return



if __name__ == "__main__":
    main()
