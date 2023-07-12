from torch.utils.data import Dataset, DataLoader
import os
from PIL import Image
import random
import numpy as np
import torch


# Depth Datasetclass
np.random.seed(42)
torch.random.seed()
def _is_pil_image(img):
    return isinstance(img, Image.Image)


def _is_numpy_image(img):
    return isinstance(img, np.ndarray) and (img.ndim in {2, 3})


class DepthDataset(Dataset):
    os = __import__('os')

    def __init__(self, traincsv, root_dir, transform=None):
        self.traincsv = traincsv
        self.root_dir = root_dir
        self.transform = transform

    def __len__(self):
        return len(self.traincsv)

    def __getitem__(self, idx):
        #sample = self.traincsv[idx]
        #img_name = root
        image = self.root_dir
        #depth_name = sample[1]
        depth = self.traincsv
        #         depth = depth[..., np.newaxis]
        sample1 = {'image': image, 'depth': depth}

        if self.transform:  sample1 = self.transform({'image': image, 'depth': depth})
        return sample1


class Augmentation(object):
    def __init__(self, probability):
        from itertools import permutations
        self.probability = probability
        # generate some output like this [(0, 1, 2), (0, 2, 1), (1, 0, 2), (1, 2, 0), (2, 0, 1), (2, 1, 0)]
        self.indices = list(permutations(range(3), 3))
        # followed by randomly picking one channel in the list above

    def __call__(self, sample):
        image, depth = sample['image'], sample['depth']

        if not _is_pil_image(image):
            raise TypeError(
                'img should be PIL Image. Got {}'.format(type(image)))
        if not _is_pil_image(depth):
            raise TypeError(
                'img should be PIL Image. Got {}'.format(type(depth)))

        # flipping the image
        if random.random() < 0.5:
            # random number generated is less than 0.5 then flip image and depth
            image = image.transpose(Image.FLIP_LEFT_RIGHT)
            depth = depth.transpose(Image.FLIP_LEFT_RIGHT)

        # rearranging the channels
        if random.random() < self.probability:
            image = np.asarray(image)
            image = Image.fromarray(image[..., list(self.indices[random.randint(0, len(self.indices) - 1)])])

        return {'image': image, 'depth': depth}


class ToTensor(object):
    def __init__(self, is_test=False):
        self.is_test = is_test

    def __call__(self, sample):
        image, depth = sample['image'], sample['depth']

        image = self.to_tensor(image)



        if self.is_test:
            depth = self.to_tensor(depth).float() / 1000
        else:
            depth = self.to_tensor(depth).float() * 1000

        # put in expected range
        depth = torch.clamp(depth, 10, 1000)

        return {'image': image, 'depth': depth}

    def to_tensor(self, pic):
        pic = np.array(pic)
        if not (_is_numpy_image(pic) or _is_pil_image(pic)):
            raise TypeError('pic should be PIL Image or ndarray. Got {}'.format(type(pic)))

        if isinstance(pic, np.ndarray):
            if pic.ndim == 2:
                pic = pic[..., np.newaxis]

            img = torch.from_numpy(pic.transpose((2, 0, 1)))

            return img.float().div(255)


















import pyk4a
import numpy as np
from PIL import Image
from pyk4a import Config, PyK4A
import torch

import cv2
from torchvision import transforms
import matplotlib.pyplot as plt
from UNETV3 import Unet, mobilenetv3_large##COMMENT OUT IF WANT TO TRY SMALL MODEL

#from UNETv3Small import Unet, mobilenetv3_small

# net = mobilenetv3_small()
# model_dict = net.state_dict()
# pretrained_dict = torch.load(r"C:\Users\Admin\Downloads\pytorch_ipynb\mobilenetv3-small-55df8e1f.pth")
# pretrained_dict = {k: v for k, v in pretrained_dict.items() if k in model_dict}
# model_dict.update(pretrained_dict)
# net.load_state_dict(model_dict)
#
#
# Model = Unet(net)



net = mobilenetv3_large()
model_dict = net.state_dict()
pretrained_dict = torch.load(r"C:\Users\benhu\SynologyDrive\Classwork\Diondre\Senior_Design_Group_FH7\UNET\host\mobilenetv3-large-1cd25616.pth")
pretrained_dict = {k: v for k, v in pretrained_dict.items() if k in model_dict}
model_dict.update(pretrained_dict)
net.load_state_dict(model_dict)


Model = Unet(net)
ModelZ = Unet(net)
ModelZ = ModelZ.cuda()
Model = Model.cuda()


LOAD_DIR = "."



# netx = Unet(net)
# model_dict = netx.state_dict()
# pretrained_dict = torch.load(r"C:\Users\Admin\Downloads\pytorch_ipynb\model.pth")
# pretrained_dict = {k: v for k, v in pretrained_dict.items() if k in model_dict}
# model_dict.update(pretrained_dict)
# netx.load_state_dict(model_dict)
#
#
# Model = netx.cuda()







Model.load_state_dict(torch.load('{}/UNET_MBIMAGENET.pth'.format(LOAD_DIR))) #imagenet best model




# Load camera with the default config
k4a = PyK4A(
    Config(
        synchronized_images_only=True,
        camera_fps= 2,

    )
)
k4a.start()
previous = [0, 0, 0, 0 ,0 ,0 , 0]
previousInput =[0, 0]


# Get the next capture (blocking function)
while True:
    capture = k4a.get_capture()

    img_color = capture.color
    img_color = cv2.resize(capture.color[240:720, 286: 926, 0:3], (640, 480))



    img_colorx = cv2.normalize(img_color, None, 0, 255, cv2.NORM_MINMAX, dtype=cv2.CV_8U)

    image = Image.fromarray(img_colorx)


    #
    # img_colorx = (previousInput[0] + img_colorx)/2
    # previousInput[0] =  img_colorx




    img_depth = cv2.resize(capture.transformed_depth[240:720, 286: 926], (640, 480))
    normed = cv2.normalize(img_depth, None, 0, 255, cv2.NORM_MINMAX, dtype=cv2.CV_8U)
    img_depth = cv2.applyColorMap(normed, cv2.COLORMAP_JET)


    depth = Image.fromarray(normed)

    depth_dataset = DepthDataset(root_dir=image, traincsv= depth, transform=transforms.Compose([ToTensor()]))

    image = torch.autograd.Variable(depth_dataset[0]['image'].cuda()).unsqueeze(0)
    #depth =  torch.autograd.Variable(sample_batched1['image'].cuda())

    # plt.imshow(image.squeeze(0).detach().cpu().numpy().T)
    # plt.show()


    output = Model(image)[0, 0, :, :].detach().cpu().numpy()
    # outputz = ModelZ(image)[0, 0, :, :].detach().cpu().numpy()
    # plt.imshow(output)
    # plt.show()
    # plt.imshow(outputz)
    # plt.show()

    #output[output < 0] = 0

    #filter
    output = (previous[0]+previous[1]+previous[2]+previous[3]+previous[4]+previous[5]+previous[6]+output)/8
    #

    previous[0] = previous[1]
    previous[1] = previous[2]

    previous[2] = previous[3]
    previous[3] = previous[4]

    previous[4] = previous[5]
    previous[5] = previous[6]

    previous[6] = output







    normedPred = cv2.normalize(output, None, 0, 255, cv2.NORM_MINMAX, dtype=cv2.CV_8U)
    normedPred = cv2.resize(normedPred, (640, 480))

    Prediction = cv2.applyColorMap(normedPred, cv2.COLORMAP_JET)
    # plt.imshow(Prediction)
    # plt.show()


    Prediction = np.flip(Prediction, axis=-1)

    # Display with pyplot
    cv2.namedWindow("Predicted")
    cv2.imshow("Predicted", Prediction)

    cv2.namedWindow("Video")
    cv2.imshow("Video", img_color)

    cv2.namedWindow("Depth")
    cv2.imshow("Depth", img_depth)


    if cv2.waitKey(1) == ord('q'):
        break
