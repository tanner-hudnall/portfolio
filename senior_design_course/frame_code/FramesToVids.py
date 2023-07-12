import cv2
import os

# Set the path to the directory containing the images
image_dir = r"D:\data\nyu2_train\SDImages"

# Set the output video file name and frames per second
output_file = r"D:\data\nyu2_train\SDImages\Output_vid.mp4"
fps = 30

# Get a list of all the image file names in the directory
image_files = sorted([os.path.join(image_dir, f) for f in os.listdir(image_dir) if f.endswith('.jpg')])

# Open the first image to get the image size
img = cv2.imread(image_files[0])
height, width, channels = img.shape

# Initialize the video writer
fourcc = cv2.VideoWriter_fourcc(*'mp4v') # Change the codec as per your requirement
out = cv2.VideoWriter(output_file, fourcc, fps, (width, height))

# Loop over all the images and add them to the video
for image_file in image_files:
    img = cv2.imread(image_file)
    out.write(img)

# Release the video writer and destroy any remaining windows
out.release()
cv2.destroyAllWindows()
