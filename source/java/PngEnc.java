
import java.awt.image.BufferedImage;
import java.awt.image.ColorModel;
import java.awt.image.DataBufferByte;
import java.awt.image.DataBufferInt;
import java.awt.image.DataBufferUShort;
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
	    	// in c 0 is success and error codes are negative values
	    	return ErrorCode.values()[-errorCode]; 
	    }
	}

	private ByteBuffer buffer;
	
	public PngEnc() {
		
	}
	
	public void save(String fileName, BufferedImage image, Strategy strategy) throws IOException, IllegalArgumentException {
		ColorModel colorModel = image.getColorModel();
		int numChannels = colorModel.getNumComponents();
		int bitDepth = image.getColorModel().getPixelSize()/numChannels;
		if (!(bitDepth == 8 || bitDepth == 16)) {
			throw new IllegalArgumentException("Unsupported color model: Must be 8 or 16 bits per pixel");
		}
		
		int capacity = image.getWidth()*image.getHeight()*colorModel.getPixelSize()/8;
		if (buffer == null || buffer.capacity() < capacity) {
			buffer = ByteBuffer.allocateDirect(capacity);
		}
		fillBuffer(image);
		
		int ret = writePngFile(fileName, buffer, image.getWidth(), image.getHeight(), numChannels, bitDepth, strategy.ordinal());
		switch (ErrorCode.get(ret)) {
			default:
			case PNGENC_ERROR:
				throw new IOException("Unknown internal error.");
				
			case PNGENC_ERROR_FAILED_NODE_DESTROY:
				throw new IOException("Internal error: Could not free memory.");
				
			case PNGENC_ERROR_FILE_IO:
				throw new IOException("Could not save png to: " + fileName);
				
			case PNGENC_ERROR_INVALID_ARG:
				throw new IllegalArgumentException("Invalid image format.");
				
			case PNGENC_SUCCESS:
				break;
		}
	}
	
	private void fillBuffer(BufferedImage image) {
		buffer.rewind();
		switch (image.getType()) {
			case BufferedImage.TYPE_USHORT_GRAY:
				buffer.asShortBuffer().put(((DataBufferUShort)image.getRaster().getDataBuffer()).getData());
				break;
			case BufferedImage.TYPE_3BYTE_BGR: {
				byte[] src = ((DataBufferByte)image.getRaster().getDataBuffer()).getData();
				byte[] dst = new byte[src.length]; // Need a copy to not modify src image
				for (int i = 0; i < src.length; i+=3) {
					// BGR -> RGB
					dst[i+2] = src[i  ];
					dst[i+1] = src[i+1];
					dst[i  ] = src[i+2];
				}
				buffer.put(dst);
				break;
			}
			case BufferedImage.TYPE_4BYTE_ABGR: {
				byte[] src = ((DataBufferByte)image.getRaster().getDataBuffer()).getData();
				byte[] dst = new byte[src.length];
				for (int i = 0; i < src.length; i+=4) {
					// ABGR -> RGBA
					dst[i  ] = src[i+3];
					dst[i+1] = src[i+2];
					dst[i+2] = src[i+1];
					dst[i+3] = src[i];
				}
				buffer.put(dst);
				break;
			}
			case BufferedImage.TYPE_INT_ARGB: {
				int[] src = ((DataBufferInt)image.getRaster().getDataBuffer()).getData();
				byte[] dst = new byte[src.length*4]; // 0xAARRGGBB
				for (int i = 0; i < src.length; i++) {
					int rgba = src[i];
					dst[4*i  ] = (byte)((rgba >> 16) & 0xFF);
					dst[4*i+1] = (byte)((rgba >> 8) & 0xFF);
					dst[4*i+2] = (byte)(rgba & 0xFF);
					dst[4*i+3] = (byte)((rgba >> 24) & 0xFF);
				}
				buffer.put(dst);
				break;
			}
			case BufferedImage.TYPE_INT_BGR: {
				int[] src = ((DataBufferInt)image.getRaster().getDataBuffer()).getData();
				byte[] dst = new byte[src.length*3];
				for (int i = 0; i < src.length; i++) {
					int rgba = src[i]; // 0x00BBGGRR
					dst[3*i  ] = (byte)(rgba & 0xFF);
					dst[3*i+1] = (byte)((rgba >> 8) & 0xFF);
					dst[3*i+2] = (byte)((rgba >> 16) & 0xFF);
				}
				buffer.put(dst);
				break;
			}
			case BufferedImage.TYPE_INT_RGB: {
				int[] src = ((DataBufferInt)image.getRaster().getDataBuffer()).getData();
				byte[] dst = new byte[src.length*3];
				for (int i = 0; i < src.length; i++) {
					int rgba = src[i]; // 0xRRGGBB
					dst[3*i  ] = (byte)((rgba >> 16) & 0xFF);
					dst[3*i+1] = (byte)((rgba >> 8) & 0xFF);
					dst[3*i+2] = (byte)(rgba & 0xFF);
				}
				buffer.put(dst);
				break;
			}
			default:
				throw new IllegalArgumentException("Unsupported color model.");
		}
		buffer.rewind();
	}

	private static native int writePngFile(String fileName, ByteBuffer buffer, int width, int height, int numChannels, int bitDepth, int strategy);
}
