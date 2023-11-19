#include <opencv2/opencv.hpp>
#include <iostream>

#define camera_index 0
//#define camera_width_px 2592
//#define camera_height_px 1944

using namespace cv;

VideoCapture videoCapture(camera_index);

class BOS {
private:
    bool pause = false;
    Mat background;

public:
    explicit BOS() {
        chequerboard();
        if (!videoCapture.isOpened()) crit_Error("Error opening video stream or file");
        get_Background();
        while (videoCapture.isOpened()) {
            // compare to last camera image instead of background
            // doing this removes the need for taking a background image, but fails to visualise static flow patterns
            //videoCapture.read(background);

            input();
            do_Schlieren();
        }
        crit_Error("camera connection closed unexpectedly");
    }

    /**
     * @brief displays a chequerboard that can be used as a BOS background
     * @param square_size size of individual squares in pixels
     * @param pixels height of chequerboard in squares (width is 2*height)
     */
    static void chequerboard(int square_size = 1, int pixels = 1024) {
        int imageSize = square_size * pixels;
        Mat chessBoard(imageSize, imageSize * 2, CV_8UC3, Scalar::all(0));
        unsigned char color = 0;

        for (int i = 0; i < imageSize * 2; i = i + square_size) {
            color = ~color;
            for (int j = 0; j < imageSize; j = j + square_size) {
                Mat ROI = chessBoard(Rect(i, j, square_size, square_size));
                ROI.setTo(Scalar::all(color));
                color = ~color;
            }
        }
        imshow("Chess board", chessBoard);
    }

    /**
     * @brief generates a BOS signal by comparing a frame from a camera to `background`
     *
     * displays the BOS signal after filtering it to improve contrast
     */
    void do_Schlieren() {
        Mat frame;
        if (!videoCapture.read(frame)) crit_Error("No frames in buffer, camera may have disconnected");

        Mat diff_img; // shows differences between frames → schlieren
        absdiff(frame, this->background, diff_img);

        Mat thresh_img; // use a threshold to reduce background noise
        threshold(diff_img, thresh_img, 20, 255, THRESH_TOZERO);
        threshold(thresh_img, thresh_img, 60, 255, THRESH_TOZERO_INV);
        thresh_img = thresh_img * 4;

        Mat temporal_img; // use a temporal filter to hide noise
        static int count = 1;
        addWeighted(diff_img, 1 - 1.0 / count, thresh_img, 1.0 / count, 0, temporal_img);
        absdiff(thresh_img, temporal_img, temporal_img);
        count++;

        Mat greyscale_img; // greyscale image
        cvtColor(thresh_img, greyscale_img, COLOR_BGR2GRAY);

        Mat clahe_img; // use a clahe to amplify signal
        cv::Ptr<CLAHE> clahe = createCLAHE(6);
        clahe->apply(greyscale_img, clahe_img);

        Mat blur_img; // apply a blur filter to further hide noise
        blur(clahe_img, blur_img, {5, 5});

        imshow("Schlieren Image", blur_img * 2);
    }

    /**
     * @brief takes an Image to be used as a background for BOS
     *
     * saves a frame from the camera to `background`
     */
    void get_Background() {
        Mat frame;
        while (!pause) {
            if (!videoCapture.read(frame)) crit_Error("No frames in buffer, camera may have disconnected");
            this->background = frame.clone();
            putText(frame, "Press space or p to capture image for background", {10, 50}, FONT_HERSHEY_SIMPLEX, 1,
                    {0, 255, 0, 255}, 3);
            imshow("Capture background", frame);
            input();
        }
        pause = false;
        destroyWindow("Capture background");
    }

    void input() {
        switch (waitKey(1)) {
            case 27: // escape
            case 'q':
            case 'Q':
                // OpenCV has nothing to handle closing a window with the 'x'-button, so this will have to do
                clean_Exit();
                break;

            case 'p':
            case 'P':
            case 32: // space bar
                pause = !pause;
                break;
        }
    }

    static void crit_Error(const std::string &errMsg) {
        std::cerr << errMsg << std::endl;
        clean_Exit(1);
    }

    static void clean_Exit(int status = 0) {
        videoCapture.release();
        destroyAllWindows();
        exit(status);
    }
};

int main() {
#if camera_height_px > 0
    videoCapture.set(CAP_PROP_FRAME_HEIGHT, camera_height_px);
#endif
#if camera_width_px > 0
    videoCapture.set(CAP_PROP_FRAME_WIDTH, camera_width_px);
#endif

    BOS bos;
}