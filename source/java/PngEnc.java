
import java.awt.image.BufferedImage;
import java.awt.image.DataBufferByte;
import java.io.IOException;
import java.nio.ByteBuffer;

public class PngEnc {
	static {
		System.loadLibrary("pngenc4j");
	}
	
	public enum Strategy {
		PNGENC_NO_COMPRESSION,
		PNGENC_HUFFMAN_ONLY_WITH_PNG_ROW_FILTER1
	}
	
	private enum ErrorCode {
	    PNGENC_SUCCESS,
	    PNGENC_ERROR,
	    PNGENC_ERROR_FILE_IO,
	    PNGENC_ERROR_INVALID_ARG,
	    PNGENC_ERROR_FAILED_NODE_DESTROY;
	    
	    public static ErrorCode get(int errorCode) {
	    	return ErrorCode.values()[-errorCode];
	    }
	}

	private ByteBuffer buffer;
	
	public PngEnc() {
		
	}
	
	public void save(String fileName, BufferedImage image, Strategy strategy) throws IOException {
		int capacity = image.getWidth()*image.getHeight()*(image.getColorModel().getPixelSize()+7)/8;
		if(buffer == null || buffer.capacity() < capacity) {
			buffer = ByteBuffer.allocateDirect(capacity);
		}
		buffer.rewind();
		buffer.put(((DataBufferByte)image.getRaster().getDataBuffer()).getData());
		buffer.rewind();
		
		if(image.getColorModel().getPixelSize() % 8 != 0) {
			throw new RuntimeException("Invalid color model");
		}
		
		int ret = writePngFile(fileName, buffer, image.getWidth(), image.getHeight(), image.getColorModel().getPixelSize()/8, strategy.ordinal());
		switch(ErrorCode.get(ret)) {
			default:
			case PNGENC_ERROR:
				throw new RuntimeException("Unknown error");
				
			case PNGENC_ERROR_FAILED_NODE_DESTROY:
				throw new RuntimeException("Saving png failed!");
				
			case PNGENC_ERROR_FILE_IO:
				throw new IOException("Could not save png to: " + fileName);
				
			case PNGENC_ERROR_INVALID_ARG:
				throw new IllegalArgumentException("Invalid image file");
				
			case PNGENC_SUCCESS:
				break;
		}
	}
	
	private static native int writePngFile(String fileName, ByteBuffer buf, int W, int h, int c, int strategy);
}
