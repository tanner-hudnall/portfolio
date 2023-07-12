import socket
import zipfile
import os
import argparse
from UNETv3_TrainFunc import TrainingLoop 

#verbosity = 0
#host_ip = "192.168.1.146"
#host_port = 8000

#nano_ip = "192.168.1.147" 
#nano_port = 6001
#jetson_num = 1 ## CHANGE IF NEEDED 

#nano_dir = r"/home/nano/srdsg/Senior_Design_Group_FH7/UNET"
nano_dir = r"/home/nano/senior_design"

def send_message(ip_address, port, message):
    # create a socket object
    sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    sock.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)



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
    #print("{}", format(data.decode))
    # return the message as a string
    return data.decode()

def arg_parser():
    # python host.py -host hostip -ip1 ip1 -ip3 ip3 -ip4 ip4 -ip5 ip5 -verbosity #
    parser = argparse.ArgumentParser(description='nano.py, Jetson Nano code for federated learning, to connect to host computer')
    parser.add_argument('-host', '--host', type=str, required=True, help='host ip')
    parser.add_argument('-ip', '--ip', type=str, required=True, help='jetson ip (this device)')
    parser.add_argument('-n', '--number', type=str, required=True, help='device number (1,2,3,4,5)')
    parser.add_argument('-v', '--verbosity', type=int, default=0, help='verbosity: 0 - only error msgs, 1 - main msgs, 2 - detailed msgs (default 0)')

    return parser.parse_args()

def main():
    
    arguments = arg_parser()
    #  error statements for no ips -- DONE
    host_ip = arguments.host
    nano_ip = arguments.ip
    jetson_num = arguments.number
    verbosity = arguments.verbosity
    

    nano_port = 8000
    host_port = int("600" + str(jetson_num))

    trained_model_file_name = f"nano_{jetson_num}_trained_model.pth"


    print("---START OF NANO.PY---\n")
    print(f"host ip: {host_ip}")
    print(f"nano ip: {nano_ip}")
    print(f"host port: {host_port}")
    print(f"device number: {jetson_num}")
    print(f"verbosity level: {verbosity}")
    

    if(verbosity >= 1):
        print("[+] initalizing workspace")
    # deletes zip files and pth files in the host dir

    if os.path.exists(f"{nano_dir}/global_mode.zip"):
        os.remove(f"{nano_dir}/global_mode.zip") 
        if(verbosity >= 2):
            print("\t[++] global_model.zip has been deleted from host\n")

    if os.path.exists(rf"{nano_dir}/UNET_MBIMAGENET.pth"):
        os.remove(rf"{nano_dir}/UNET_MBIMAGENET.pth")
        if(verbosity >= 2):
            print("\t[++] UNET_MBIMAGENET.pth has been deleted from host\n")

    if os.path.exists(trained_model_file_name):
        os.remove(trained_model_file_name)
        if(verbosity >= 2):
            print("\t[++] trained_model.pth has been deleted from host\n")

    if os.path.exists(f"trained_model_{jetson_num}.zip"):
        os.remove(f"trained_model_{jetson_num}.zip")
        if(verbosity >= 2):
            print(f"\t[++] trained_model_{jetson_num}.zip has been deleted from host\n")

    while True:
        
        # #TODO change this if time permits otherwise just leave :)
        # if(verbosity >= 2):
        #     print("\t[++] creating/resetting a folder for jetson\n")
        # folder_name = "jetson_{}".format(jetson_num) 
        # current_dir = os.getcwd()
        # new_dir = os.path.join(current_dir, folder_name)
        # if not os.path.exists(new_dir):
        #     os.makedirs(new_dir)
        #     if(verbosity >= 2):
        #         print(f"\t[++] Directory {new_dir} created successfully\n")
        # else:
        #     if(verbosity >= 2):
        #         print(f"\t[++] Directory {new_dir} already exists\n")
        #     os.removedirs(new_dir)
        #     os.makedirs(new_dir)




        print("waiting for message from host laptop...\n\tif done with federated learning you may terminate the code")
        while True:
            if receive_message(nano_ip,nano_port) == "connected":
                break
        if(verbosity >= 2):
            print("\t[++] recieved 'connected' message from host\n")   


        print("\t[++] sending host an acknowledgement message\n")
        send_message(host_ip,host_port,"ACK")
        #print("'ACK' message sent")

        
        if(verbosity >= 2):
            print("\t[++] waiting for a message from the host laptop informing the nano that global model was sent...\n")
        while True:
            if receive_message(nano_ip, nano_port) == "global_model_sent":
                break
        if(verbosity >= 2):
            print("\t[++] received global 'global_model_sent'\n")


        #print("getting the size of the global model on the nano side\n")        
        
        file_size = os.path.getsize(rf"{nano_dir}/global_model.zip") 
        #print("file size on the nano # side is {} bytes\n".format(file_size))

        #print("sending a number representing the size of the global model to host\n")
        send_message(host_ip, host_port,"{}".format(file_size))
        #print("file size sent to host\n")


        print("\t[++] waiting for message from host to begin training...\n")
        while(True):
            if(receive_message(nano_ip,nano_port) == "start_train"):
                break
        if(verbosity >= 2):
            print("\t[++] 'start_train' message has been received \n")

        # folder_name = "jetson_{}".format(jetson_num) 
        # current_dir = os.getcwd()
        # folder_path = os.path.join(current_dir, folder_name)

        # if os.path.exists(folder_path):
        #     os.chdir(folder_path)
        #     if(verbosity >= 2):
        #         print(f"\t[++] Changed working directory to {folder_path}")
        # else:
        #     print(f"[!] Directory {folder_path} does not exist")

        #print("extract the global model from its zip file\n")
        with zipfile.ZipFile(f"{nano_dir}/global_model.zip", 'r') as zip_ref: 
            zip_ref.extractall(nano_dir)
            os.remove(f"{nano_dir}/global_model.zip") 
        if(verbosity >= 2):
            print("\t[++] global_model.zip has been extracted for training loop and subsequently deleted\n") 

        # current_dir = os.getcwd()
        # if os.path.basename(current_dir) == "jetson_{}": 
        #     os.chdir('..')
        #     if(verbosity >= 2):
        #         print(f"\t[++] Exited {current_dir}, current working directory is now {os.getcwd()}")
        # else:
        #     print(f"[!] Error: current directory is not jetson_{jetson_num}") 

        #print("setting name of trained model file\n")
        
        #print("\t[++] name of trained model is {}\n".format(trained_model_file_name))

        if(verbosity >= 1):
            print("[+] starting to training model on nano\n")
        model = TrainingLoop(2, 
                             1, 
                             .0001, 
                             trained_model_file_name,
                             jetson_num=jetson_num, 
                             verbosity=verbosity)
        
        if(verbosity >= 2):
            print("\t[++] training of model complete\n")

        #print("zipping the trained model on nano\n")
        with zipfile.ZipFile(f"trained_model_{jetson_num}.zip", mode = 'w') as archive: 
            archive.write(trained_model_file_name)
        if(verbosity >= 2):
            print("\t[++] zipping of trained model complete\n")


        #print("sending a message to host that the model has finished training\n")
        send_message(host_ip,host_port,"train_finish")
        if(verbosity >= 2):
            print("\t[++] 'train_finish' message sent to host\n")


        #print("waiting for a meesage from the host that the trained model was retrieved by the host\n")
        while(True):
            if(receive_message(nano_ip,nano_port) == "model_sent_back"):
                break
        #print("'model_sent_back' message has been received by nano\n")


        #print("getting size of the trained model on the nano side\n")
        file_size = os.path.getsize(r"trained_model_{}.zip".format(jetson_num)) 
        #print("file size of the trained model is {} bytes\n".format(file_size))

        #print("sending a number to the host laptop reprensenting the size of the trained model on the nano side\n")
        send_message(host_ip,host_port,"{}".format(file_size))
        #print("file size of trained model has been sent to host")

        if(verbosity >= 2):
            print("\t[++] removing trained model from the nano side\n")
        os.remove(trained_model_file_name)
        os.remove(f"trained_model_{jetson_num}.zip")
        
        #print("---END OF NANO CODE---\n")

    return



if __name__ == "__main__":
    main()