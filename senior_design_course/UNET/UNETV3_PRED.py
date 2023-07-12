from UNETV3 import Unet, mobilenetv3_large
import cv2
import kornia

import kornia.losses.ssim
from DepthDataset import DepthDataset, Augmentation, ToTensor

Model = Unet(mobilenetv3_large())

def SSIM(img1, img2, val_range, window_size=11, window=None, size_average=True, full=False):
    #ssim = kornia.losses.ssim(window_size=11, max_val=val_range, reduction='none')
    return kornia.losses.ssim_loss(img1, img2,window_size=11, max_val=val_range, reduction='none')
# SSIM = kornia.losses.ssim(window_size=11, max_val=val_range, reduction='none')

import matplotlib
import matplotlib.cm
import numpy as np
import matplotlib.pyplot as plt


def DepthNorm(depth, maxDepth=1000.0):
    return maxDepth / depth


class AverageMeter(object):
    def __init__(self):
        self.reset()

    def reset(self):
        self.val = 0
        self.avg = 0
        self.sum = 0
        self.count = 0

    def update(self, val, n=1):
        self.val = val
        self.sum += val * n
        self.count += n
        self.avg = self.sum / self.count


def colorize(value, vmin=10, vmax=1000, cmap='plasma'):
    value = value.cpu().numpy()[0, :, :]

    # normalize
    vmin = value.min() if vmin is None else vmin
    vmax = value.max() if vmax is None else vmax
    if vmin != vmax:
        value = (value - vmin) / (vmax - vmin)  # vmin..vmax
    else:
        # Avoid 0-division
        value = value * 0.
    # squeeze last dim if it exists
    # value = value.squeeze(axis=0)

    cmapper = matplotlib.cm.get_cmap(cmap)
    value = cmapper(value, bytes=True)  # (nxmx4)

    img = value[:, :, :3]

    return img.transpose((2, 0, 1))


def LogProgress(model, writer, test_loader, epoch):
    model.eval()
    sequential = test_loader
    sample_batched = next(iter(sequential))
    image = torch.autograd.Variable(sample_batched['image'].cuda())
    depth = torch.autograd.Variable(sample_batched['depth'].cuda(non_blocking=True))
    if epoch == 0: writer.add_image('Train.1.Image', vutils.make_grid(image.data, nrow=6, normalize=True), epoch)
    if epoch == 0: writer.add_image('Train.2.Depth', colorize(vutils.make_grid(depth.data, nrow=6, normalize=False)),
                                    epoch)
    output = DepthNorm(model(image))
    writer.add_image('Train.3.Ours', colorize(vutils.make_grid(output.data, nrow=6, normalize=False)), epoch)
    writer.add_image('Train.3.Diff',
                     colorize(vutils.make_grid(torch.abs(output - depth).data, nrow=6, normalize=False)), epoch)
    del image
    del depth
    del output


import time
import argparse
import datetime
import pandas as pd
import torch
import os
import cv2
from sklearn.utils import shuffle
import torch.nn as nn
import torch.nn.utils as utils
import torchvision.utils as vutils
from torch.utils.data import Dataset, DataLoader
from torchvision import transforms, utils
from tensorboardX import SummaryWriter

# from data import getTrainingTestingData
# from utils import AverageMeter, DepthNorm, colorize

model = Model.cuda()
if torch.cuda.device_count() > 1:
    print("Let's use", torch.cuda.device_count(), "GPUs!")
    # dim = 0 [30, xxx] -> [10, ...], [10, ...], [10, ...] on 3 GPUs
    model = nn.DataParallel(model)
# load trained model if needed
LOAD_DIR = r"C:\Users\Admin\Downloads\pytorch_ipynb"
model.load_state_dict(torch.load('{}/UNET_MBIMAGENET.pth'.format(LOAD_DIR)))
print('Model Loaded.')

epochs = 50
lr = 0.0001
batch_size = 1

traincsv= pd.read_csv(r"C:\Users\Admin\Downloads\pytorch_ipynb\data\nyu2_train.csv")
traincsv = traincsv.values.tolist()
traincsv = shuffle(traincsv, random_state=2)



depth_dataset = DepthDataset(traincsv=traincsv, root_dir=r"C:\Users\Admin\Downloads\pytorch_ipynb",
                             transform=transforms.Compose([ ToTensor()]))
train_loader = DataLoader(depth_dataset, batch_size, shuffle=False)
l1_criterion = nn.MSELoss()

optimizer = torch.optim.Adam(model.parameters(), lr, amsgrad= True)
loss_list = []
epoch_list = []
DEVICE = torch.device("cuda" if torch.cuda.is_available() else "cpu")
# Start training...
model = Model.cuda()
model = nn.DataParallel(model)
# load the model if needed
# model.load_state_dict(torch.load('/workspace/3.pth'))
model.eval()
batch_size = 1


# for i, sample_batched1 in enumerate(train_loader):
#     image1 = torch.autograd.Variable(sample_batched1['image'].cuda())
#
#     outtt = model(image1)
#     x = outtt.detach().cpu().numpy()
#     resized = x.reshape(480, 640)
#
#     plt.imsave('{}/generated_img/%d_depth.jpg'.format(LOAD_DIR) % i, resized ,cmap = "viridis_r")
#
#     s_img = sample_batched1['image'].detach().cpu().numpy()
#     s_img = s_img[0,:,:,:]
#     s_img = s_img.swapaxes(0, 1)
#     s_img = s_img.swapaxes(1, 2)
#     plt.imshow(s_img)
#     plt.show()
#     plt.imsave('{}/generated_img/%d_image.jpg'.format(LOAD_DIR) % i, s_img)
#
#     s_img = sample_batched1['depth'].detach().cpu().numpy()
#     s_img = s_img[0, 0, :, :]
#     #s_img = s_img.reshape(480, 640)
#     plt.imshow(s_img)
#     plt.show()
#
#     plt.imsave('{}/generated_img/%d_GTDepth.jpg'.format(LOAD_DIR) % i, s_img)
input = plt.imread(r"C:\Users\Admin\Downloads\pytorch_ipynb\data\nyu2_train\SDImages\frame_000001.jpg")
convert = transforms.ToTensor()
input = convert(input)
input = input.unsqueeze(0)
output = model(input)[0,0,:,:]

x = output.detach().cpu().numpy()
plt.imshow(x)
plt.show()

