import cv2
import os
import argparse
import numpy as np
from glob import glob  # For sorting filenames

def min_max_stretch(image):
  """
  Performs min-max stretch on an image.
  """
  min_val = np.min(image)
  max_val = np.max(image)
  return 65535 * ((image - min_val) / (max_val - min_val))  # Scale to 0-

def create_avi(image_dir, fps, output_filename="output.avi"):
  """
  Converts all TIFF images in a directory to an AVI video with normalization.

  Args:
      image_dir: Path to the directory containing TIFF images.
      fps: Frames per second for the output AVI video.
      output_filename: Name of the output AVI file (default: output.avi).
  """
  # Get all TIFF files in the directory
  image_files = sorted(glob(os.path.join(image_dir, "*.tiff")), key=os.path.basename)
  # Check if any images found
  if not image_files:
    print("No TIFF images found in the directory!")
    return

  # Read the first image to get dimensions
  first_image = cv2.imread(image_files[0], cv2.IMREAD_GRAYSCALE)  # Assuming grayscale images
  height, width = first_image.shape

  output_filename = os.path.splitext(os.path.basename(image_dir))[0] + "output.avi"  # Extract directory name and add .avi
  # Create video writer
  fourcc = cv2.VideoWriter_fourcc(*'XVID')  # Adjust codec if needed
  video = cv2.VideoWriter(os.path.join(image_dir, output_filename), fourcc, fps, (width, height))

  # Process each image and write to video
  for image_file in image_files:
    image = cv2.imread(image_file, cv2.IMREAD_GRAYSCALE)
    normalized_image = min_max_stretch(image).astype(np.uint8)  # Apply min-max stretch and convert to uint8    video.write(cv2.cvtColor(normalized_image, cv2.COLOR_GRAY2BGR))  # Convert to BGR for color video
    video.write(cv2.cvtColor(normalized_image, cv2.COLOR_GRAY2BGR))  # Convert to BGR for color video
  # Release resources
  video.release()
  print(f"AVI video created: {os.path.join(image_dir, output_filename)}")
if __name__ == "__main__":
  # Parse command line arguments
  parser = argparse.ArgumentParser(description="Convert TIFF images to AVI with normalization.")
  parser.add_argument("image_dir", help="Path to the directory containing TIFF images.")
  parser.add_argument("fps", type=int, help="Frames per second for the output AVI video.")
  args = parser.parse_args()

  create_avi(args.image_dir, args.fps)