# Created by: Diondre Dubose (01/23/23)
# Edited by: 
#  The code converts a set of .mkv files into frames 

# Import dependecies
import os
from zipfile import ZipFile #unzips zip files
import cv2
import torch
from torch.utils.data import Dataset
import numpy as np
import subprocess
import open3d as o3d
import cv as mkv_reader
import matplotlib.pyplot as plt
from matplotlib import image
import torchvision.transforms as transforms
from PIL import Image




"""
Ability 1: Enter folder
Ability 2: Exit folder
Ability 3: Create folder
Ability 4: Extract .zip files
Ability 5: Extract the RGB image 
           and depth map from each
           frame of a .mkv video file
"""



def enter_folder(folder_name):
    #gets current working directory (cwd)
    cwd = os.getcwd()

    #create folder path
    folder_path = os.path.join(cwd, folder_name)

    # go to directory specified by folder path
    os.chdir(folder_path)

    #prints the name of folder that is entered
    print(" \n --- '{}' folder has been ENTERED --- \n".format(folder_name))

def exit_folder():
    #gets current working directory (cwd)
    cwd = os.getcwd()

    #path of parent directory (pd)
    # os.path.dirname('/home/user/Documents/my_folder') would return /home/user/Documents
    pd_path = os.path.dirname(cwd)

    # go to directory specified by pd_path
    os.chdir(pd_path)
    
    folder_name = cwd.rsplit('\\', 1)[1]
    print(" \n --- '{}' folder has been EXITED --- \n".format(folder_name))

def create_folder(folder_name):
    #gets current working directory (cwd)
    cwd = os.getcwd()

    folder_path = os.path.join(cwd, folder_name)
    if os.path.isdir(folder_path):
        print("\n --- '{}' folder already exists, skipping creation process --- \n".format(folder_name))
    else:
        os.mkdir(folder_path)
        print(" \n --- '{}' folder has been CREATED --- \n".format(folder_name))


def extract_zip(file_name):
    # check if the folder with the same name as the zip file already exists
    folder_name = file_name.rsplit('.',1)[0]
    if os.path.isdir(folder_name):
        print("\n --- Extraction of '{}' already exists, skipping extraction process --- \n".format(file_name))
        return
    with ZipFile(file_name, 'r') as zip:
        print("\n Extracting '{}' ... \n".format(file_name))
        zip.extractall()
        print("\n --- Extraction of '{}' Complete --- \n".format(file_name))


def extract_zip_in_folder(file_name, folder_name):
   #if os.path.isdir(folder_name):
        #print("\n --- Extraction of '{}' already exists, skipping extraction process --- \n".format(file_name))
        #return

    with ZipFile(file_name, 'r') as zip:
        print("\n Extracting '{}' ... \n".format(file_name))
        create_folder(folder_name)
        enter_folder(folder_name)
        zip.extractall()
        exit_folder()
        print("\n --- Extraction of '{}' Complete --- \n".format(file_name))


def video_expander(video_file):
    # Extract the RGB image and depth map from each frame of a .mkv video file
    where_video_is = os.getcwd()
    
    folder_name = video_file.rsplit('.',1)[0]
    create_folder(folder_name)
    enter_folder(folder_name)

    create_folder("RGB_Images")
    create_folder("Depth_Maps")
    READ_FRAME = mkv_reader.ReaderWithCallback(os.path.join(where_video_is ,video_file),os.getcwd())
    READ_FRAME.run()
    now = os.getcwd()
    exit_folder()


def frame_generator(zip_file):
    folder_name = zip_file.rsplit('.',1)[0]   
    while(True):
        if '.' in folder_name:
            folder_name = folder_name.rsplit('.',1)[0]
        else:
            break

    extract_zip(zip_file)
    enter_folder(folder_name)
    
    cwd = os.getcwd()
    FILES = [file for file in os.listdir(cwd) if file.endswith(".mkv")]
    for video_file in FILES:
        if not os.path.exists(os.path.join(cwd, video_file.rsplit('.',1)[0])):
            video_expander(video_file)
        else:
            print("{} has already been extracted".format(video_file))
    exit_folder()

class AgentDataset(Dataset):
    def __init__(self, zip_file, transform):
        frame_generator(zip_file)
        self.transform = transform
        folder_name = zip_file.rsplit('.',1)[0]   
        while(True):
            if '.' in folder_name:
                folder_name = folder_name.rsplit('.',1)[0]
            else:
                break
        cwd = os.getcwd()
        cwd = os.path.join(cwd, folder_name)
        self.array = []
        oldcwd = cwd
        for file in os.listdir(cwd):
            if file.endswith(".mkv"):
                i = file.rsplit('.',1)[0]   
                while(True):
                    if '.' in i:
                        i = i.rsplit('.',1)[0]
                    else:
                        break
                self.array.append(i)
        self.rgb_images = [] 
        self.depth_maps = []
        self.folder_indices = []
        start_index = 0
        for folder in self.array:  
            cwd = oldcwd
            cwd = os.path.join(cwd, folder)
            if os.path.isdir(os.path.join(cwd, "RGB_Images")) and os.path.isdir(os.path.join(cwd, "Depth_Maps")):
                self.folder_indices.append(start_index)
                self.x = [os.path.join(cwd,"RGB_Images", file) for file in os.listdir(os.path.join(cwd, "RGB_Images")) if file.endswith(".jpg")]
                self.y = [os.path.join(cwd, "Depth_Maps", file) for file in os.listdir(os.path.join(cwd, "Depth_Maps")) if file.endswith(".png")]
                start_index += len(self.x)
                self.rgb_images += self.x
                self.depth_maps += self.y
    def __len__(self):
        return len(self.rgb_images)
    def __numclips__(self):
        return len(self.folder_indices)

    def __getitem__(self, idx, clip=0):
        rgb_img = Image.open(self.rgb_images[(idx+self.folder_indices[clip])%(len(self.rgb_images))])
        depth_map = Image.open(self.depth_maps[(idx+self.folder_indices[clip])%(len(self.depth_maps))])
        if self.transform:
            rgb_img = self.transform(rgb_img)
            depth_map = self.transform(depth_map)
        return np.array(rgb_img), np.array(depth_map)

#transform = transforms.Compose([transforms.RandomHorizontalFlip(),transforms.RandomRotation(degrees=45)])
ECJ = AgentDataset("roomrecordings_2023_01_22.zip", transform=None)
num_frames = ECJ.__len__()
for frame in range(ECJ.__numclips__()):
    #clip parameter is used to skip to start indexing at specific clip
    image, depth = ECJ.__getitem__(idx=0, clip=frame)
    fig, (ax1, ax2) = plt.subplots(1, 2, figsize=(10, 5))
    ax1.imshow(depth, cmap='jet')
    ax1.set_title("Depth Image")
    ax2.imshow(image)
    ax2.set_title("RGB Image")
    fig.suptitle("Depth and RGB Images")
    plt.show()
   
dataloader = torch.utils.data.DataLoader(ECJ, batch_size=32, shuffle=True)
