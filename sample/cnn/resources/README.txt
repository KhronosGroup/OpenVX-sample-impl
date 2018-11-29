Prerequisites for test execution:
1. Download Overfeat weights file from: http://cilvr.nyu.edu/lib/exe/fetch.php?media=overfeat:overfeat-weights.tgz
2. Run the cnn_convert app on the downloaded net_weight_0 file to convert float weights to Q1.7.8 shorts. 
3. Copy the result file to the executable's folder. It will be used as the weights file for the Overfeat CNN network.
4. Add openvx and cnn_network library path to LD_LIBRARY_PATH

Additional prerequisites for sample app execution:
1. Download, build and install opencv from http://opencv.org/downloads.html. Make sure the relevant image format read/write library is included (e.g. BUILD_JPEG flag is set) 
2. Define an environment variable - OPENCV_INSTALL_PATH - that will point to the OpenCV installation path
3. Add OPENCV_INSTALL_PATH library path to LD_LIBRARY_PATH

cnn_convert app usage:
cnn_convert -bf2short <input_file> <output_file>
input_file should contain float weights per layer. For each layer the weights will be followed by the biases.

cnn_sample_app usage:
cnn_sample_app <network_type> -r -i <input_file> <mean_file> <weight_file> <classes_file>
network_type - "overfeat" or "alexnet"
-r - raw unprocessed image. 
	 Without this flag the image should be an RGB planar Q1.7.8 binary after preprocessing (average reduction and division by sd if needed).
	 Overfeat expects 231x231x3, Alexnet expects 256x256x3.
-i - OpenVX immediate mode. 
	 Without this flag a full graph will be built and processed.
mean_file - only for Alexnet. Should be in RGB planar Q1.7.8 binary format.
weight_file - should be in RGB planar Q1.7.8 binary format.
classes_file - a text file containing the 1000 classification options

In addition to screen display, results will be dumped into an app_result.txt file under the resources folder.