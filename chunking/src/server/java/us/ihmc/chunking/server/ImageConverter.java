package us.ihmc.chunking.server;


import javax.imageio.ImageIO;
import java.awt.image.BufferedImage;
import java.io.ByteArrayInputStream;
import java.io.ByteArrayOutputStream;
import java.io.IOException;

/**
 * Created by gbenincasa on 6/27/17.
 */
public class ImageConverter {

    enum Format {
        BMP,
        JPG,
        PNG
    }

    static BufferedImage convert (BufferedImage input, Format outputFormat) throws IOException {
        ByteArrayInputStream in = null;
        ByteArrayOutputStream out = new ByteArrayOutputStream();
        try {
            if (ImageIO.write(input, outputFormat.name().toLowerCase(), out)) {
                in = new ByteArrayInputStream(out.toByteArray());
                return ImageIO.read(in);
            }
        }
        finally {
            try {
                out.close();
                if (in != null) in.close();
            }
            catch(Exception e) {}
        }
        throw new IOException("could not read source image or write output "
                + outputFormat.name().toLowerCase() + " image");
    }
}
