package org.dg.camera;

import java.io.BufferedReader;
import java.io.FileNotFoundException;
import java.io.FileReader;
import java.util.ArrayList;
import java.util.LinkedList;
import java.util.List;
import java.util.Locale;
import java.util.Scanner;
import java.util.concurrent.Semaphore;

import org.dg.openAIL.MapPosition;
import org.opencv.android.Utils;
import org.opencv.core.Mat;
import org.opencv.core.Point3;

import com.google.zxing.BinaryBitmap;
import com.google.zxing.LuminanceSource;
import com.google.zxing.RGBLuminanceSource;
import com.google.zxing.Result;
import com.google.zxing.common.HybridBinarizer;
import com.google.zxing.qrcode.QRCodeReader;

import android.content.Context;
import android.graphics.Bitmap;
import android.os.AsyncTask;
import android.util.Log;
import android.util.Pair;
import android.widget.Toast;

public class QRCodeDecoderClass {
    private final String TAG = "QRCode";
    
    private Context mContext;
    
    private List<Pair<Integer,String>> recognizedMessages = null;
    
    private final Semaphore recognizedMessagesMtx = new Semaphore(1, true);
    
    public QRCodeDecoderClass (Context context) {
    	mContext = context;
    	recognizedMessages = new LinkedList<Pair<Integer,String>>();
    }
	
	public void decode(Integer positionId, Mat image) {
		Log.d(TAG, "creating async task");
		new DecodeAsyncTask(positionId, mContext).execute(image);
	}
	
	public List<Pair<Integer,Point3>> getRecognizedQRCodes() {
		
		List<Pair<Integer,Point3>> returnList = new LinkedList<Pair<Integer,Point3>>();
		
		try {
			
			
			
			recognizedMessagesMtx.acquire();
			for (Pair<Integer, String> pair : recognizedMessages) {
				
				// Preparations to read positions
				Scanner positionScanner = null;
				positionScanner = new Scanner(pair.second);
				positionScanner.useLocale(Locale.US);
			
				
				positionScanner.next("Position");
				double X = positionScanner.nextDouble();
				double Y = positionScanner.nextDouble();
				double Z = positionScanner.nextDouble();
				
				Point3 pos = new Point3(X, Y, Z);
				returnList.add(new Pair<Integer,Point3>(pair.first, pos));
			}
			recognizedMessages.clear();
			
			recognizedMessagesMtx.release();
			
		} catch (Exception e) {
			e.printStackTrace();
		}
		
		
		
		return returnList;
		
	}
	
	private class DecodeAsyncTask extends AsyncTask<Mat, Void, String> {
		
		Integer positionId = 0;
		
		Context context;

        /**
         * @param context
         */
        private DecodeAsyncTask(Integer _positionId, Context _context) {
        	context = _context;
        	positionId = _positionId;
        }
        
        @Override
        protected String doInBackground(Mat... image) {
            return decodeQRImage(image[0]);
        }
        

        @Override
        protected void onPostExecute(String result) {
        	Log.d(TAG, "onPostExecute : " + result);
        	Toast.makeText(context, result, Toast.LENGTH_LONG).show();
        	
        	if ( result != "Decode failed!") {
				try {
					recognizedMessagesMtx.acquire();
					
					recognizedMessages.add(new Pair<Integer,String>(positionId, result));
		        	
					recognizedMessagesMtx.release();
				} catch (InterruptedException e) {
					e.printStackTrace();
				}
        	}

        }
        
        private String decodeQRImage(Mat image) {
        	Log.d(TAG, "decodeQRImage in async");
    		Bitmap bMap = Bitmap.createBitmap(image.width(), image.height(), Bitmap.Config.ARGB_8888);
    		Utils.matToBitmap(image, bMap);
    		int[] intArray = new int[bMap.getWidth()*bMap.getHeight()];  
    		
    		//copy pixel data from the Bitmap into the 'intArray' array  
    		bMap.getPixels(intArray, 0, bMap.getWidth(), 0, 0, bMap.getWidth(), bMap.getHeight());  

    		LuminanceSource source = new RGBLuminanceSource(bMap.getWidth(), bMap.getHeight(),intArray);

    		BinaryBitmap bitmap = new BinaryBitmap(new HybridBinarizer(source));
    		QRCodeReader reader = new QRCodeReader();  
    		
    		//....doing the actually reading
    		Result result = null;
    		try {
    			result = reader.decode(bitmap);
    		}
    		catch (Exception e) {
    			return "Decode failed!";
    		}
    		
    		return result.toString();
    	}

      
    
	}
	
	

}
