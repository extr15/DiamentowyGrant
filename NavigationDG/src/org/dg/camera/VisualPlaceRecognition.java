package org.dg.camera;
import java.io.File;
import java.util.Locale;

import org.opencv.core.Mat;
import org.opencv.highgui.Highgui;
import org.opencv.imgproc.Imgproc;

import android.os.Environment;
import android.util.Log;

public class VisualPlaceRecognition {
	
	private static final String moduleLogName = "VisualPlaceRecognitionJava";
	
	static {
		try {
		System.loadLibrary("VisualPlaceRecognitionModule");
		} catch (UnsatisfiedLinkError e)
		{
			Log.d(moduleLogName, "Native code library failed to load.\n" + e);
		}
	}
	
	// Native methods
	private native long createAndLoadFabmapNDK(String settingsPath);
	private native long createAndTrainFabmapNDK(String settingsPath, int trainingSetSize);
	private native void addTestSetFabmapNDK(long addrFabmapEnv, int testSetSize);
	private native int testLocationFabmapNDK(long addrFabmapEnv, long addrImageToTest, boolean addToTest);
	private native void destroyFabmapNDK(long addrFabmap);
	
	/// Java code	
	
	// Fabmap object address
	long addrFabMapEnv;
	
	// Training images
	Mat[] trainImages;
	
	// Test images
	Mat[] testImages;
	
	public VisualPlaceRecognition() {
		
		//Initialize FabMapEnv pointer to NULL
		addrFabMapEnv = 0;
	}
	
	// TODO: Just for test
	public void callAndVerifyAllMethods() {
		trainVisualPlaceRecognition();
		
		{
			String fabmapTestRecPath = String.format(Locale.getDefault(), Environment
					.getExternalStorageDirectory().toString()
					+ "/OpenAIL"
					+ "/VPR/testRec/");
			
			File fTestRec = new File(fabmapTestRecPath);        
			File fileTestRec[] = fTestRec.listFiles();
			Log.d(moduleLogName, "Test rec size: " + fileTestRec.length);
			
			for (int i=0; i < fileTestRec.length; i++)
			{
			    Log.d(moduleLogName, "Train filename:" + fileTestRec[i].getName());
			    Mat testRecImage = Highgui.imread(fabmapTestRecPath + fileTestRec[i].getName(), 0);
			    
			    recognizePlace(testRecImage);
			}
		}
		
		destroyFabmapNDK(addrFabMapEnv);
	}
	
	// Method used to run the training of FABMAP
	public void trainVisualPlaceRecognition() {
		Log.d(moduleLogName, "Called trainVisualPlaceRecognition()");
		
		if(addrFabMapEnv != 0){
			destroyFabmapNDK(addrFabMapEnv);
			addrFabMapEnv = 0;
		}
		
		// Read training images
		int trainingSetSize;
		{
			String fabmapTrainPath = String.format(Locale.getDefault(), Environment
					.getExternalStorageDirectory().toString()
					+ "/OpenAIL"
					+ "/VPR/train/");
			
			File fTrain = new File(fabmapTrainPath);        
			File fileTrain[] = fTrain.listFiles();
			Log.d(moduleLogName, "Train size: " + fileTrain.length);
			trainImages = new Mat[fileTrain.length];
			
			for (int i=0; i < fileTrain.length; i++)
			{
			    Log.d(moduleLogName, "Train filename:" + fileTrain[i].getName());
			    trainImages[i] = Highgui.imread(fabmapTrainPath + fileTrain[i].getName(), 0);
			}
			trainingSetSize = fileTrain.length;
		}

		Log.d(moduleLogName, "training FabMap");
		// Fabmap settings path
		{
			String fabmapSettingsPath = String.format(Locale.getDefault(), Environment
			.getExternalStorageDirectory().toString()
			+ "/OpenAIL"
			+ "/VPR/settings.yml");
			
			// Call training of fabmap library
//			addrFabMapEnv = createAndTrainFabmapNDK(fabmapSettingsPath, trainingSetSize);
			addrFabMapEnv = createAndLoadFabmapNDK(fabmapSettingsPath);
		}
		
		// Read testimages
		int testSetSize;
		{
			String fabmapTestPath = String.format(Locale.getDefault(), Environment
					.getExternalStorageDirectory().toString()
					+ "/OpenAIL"
					+ "/VPR/test/");
			
			File fTest = new File(fabmapTestPath);        
			File fileTest[] = fTest.listFiles();
			Log.d(moduleLogName, "Test size: " + fileTest.length);
			testImages = new Mat[fileTest.length];
			
			for (int i=0; i < fileTest.length; i++)
			{
			    Log.d(moduleLogName, "Test filename:" + fileTest[i].getName());
			    testImages[i] = Highgui.imread(fabmapTestPath + fileTest[i].getName(), 0);
			}
			testSetSize = fileTest.length;
		}

		Log.d(moduleLogName, "adding test images");
		addTestSetFabmapNDK(addrFabMapEnv, testSetSize);
	}
	
	// Method used to try to match new image from camera to existing dataset of images
	public void recognizePlace(Mat imageToTest) {
		int match = testLocationFabmapNDK(addrFabMapEnv, imageToTest.getNativeObjAddr(), true);

		Log.d(moduleLogName, "Recognized place: " + match);
	}

}