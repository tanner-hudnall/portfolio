import pyk4a
from pyk4a import Config, PyK4A
import torch
import cv2
from torchvision import transforms
import matplotlib.pyplot as plt
from UNETV3 import Unet, mobilenetv3_large##COMMENT OUT IF WANT TO TRY SMALL MODEL

# from UNETv3Small import Unet, mobilenetv3_small
#
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
pretrained_dict = torch.load(r"C:\Users\Admin\Downloads\pytorch_ipynb\mobilenetv3-large-1cd25616.pth")
pretrained_dict = {k: v for k, v in pretrained_dict.items() if k in model_dict}
model_dict.update(pretrained_dict)
net.load_state_dict(model_dict)


Model = Unet(net)


LOAD_DIR = "."
Model.load_state_dict(torch.load('{}/UNET_MBIMAGENET.pth'.format(LOAD_DIR)))



# Load camera with the default config
k4a = PyK4A(
    Config(
        synchronized_images_only=True,
    )
)
k4a.start()

# Get the next capture (blocking function)
while True:
    capture = k4a.get_capture()
    img_color = capture.color
    img_color = cv2.resize(capture.color[240:720, 286: 926, 0:3], (640, 480))

    img_depth = cv2.resize(capture.transformed_depth[240:720, 286: 926], (640, 480))
    normed = cv2.normalize(img_depth, None, 0, 255, cv2.NORM_MINMAX, dtype=cv2.CV_8U)
    img_depth = cv2.applyColorMap(normed, cv2.COLORMAP_JET)

    # dont ask me how or why this works...
    ############################
    cv2.imwrite('test.jpg', img_color)
    input = plt.imread(r"C:\Users\Admin\Downloads\pytorch_ipynb\test.jpg")
    convert = transforms.ToTensor()
    input = convert(input)
    input = input.unsqueeze(0)

    ############################
    # input = torch.from_numpy(img_color)
    # input = torch.reshape(input, (1, 3, 480, 640))
    #
    #
    # output = Model(input.float())[0, 0 , : , :]

    output = Model(input)[0, 0, :, :].detach().cpu().numpy()

    normedPred = cv2.normalize(output, None, 0, 255, cv2.NORM_MINMAX, dtype=cv2.CV_8U)
    normedPred = cv2.resize(normedPred, (640, 480))
    Prediction = cv2.applyColorMap(normedPred, cv2.COLORMAP_JET)

    # plt.imshow(x)
    # plt.show()

    # Display with pyplot
    cv2.namedWindow("Predicted")
    cv2.imshow("Predicted", Prediction)

    cv2.namedWindow("Video")
    cv2.imshow("Video", img_color)

    cv2.namedWindow("Depth")
    cv2.imshow("Depth", img_depth)


    if cv2.waitKey(1) == ord('q'):
        break

