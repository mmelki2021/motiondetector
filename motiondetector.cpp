/*******************************************************************
* NAME :            motiondetector.cpp
*
* DESCRIPTION :     sample program of motiondetector
*
*/


#include <string>
#include <iostream>           // standard input/output
#include <vector>             // vector used as container
#include <algorithm>          // for_each vectors and equal
#include <thread>             // thread
#include <mutex>              // mutex, unique_lock
#include <condition_variable> // condition_variable
#include <queue>              // queue used in async queue
#include <random>             // to generate random values
#include <memory>             // shared pointers

using namespace std;

/**
 * @brief VideoFrame class to store pixels and video Frame size (width && height).
 */
class VideoFrame
{
public:
    /**
     * @brief video frame class constructor.
     * @param width(in): video frame width.
     * @param height(in): video frame Height.
     */
    VideoFrame(uint8_t &width, uint8_t &height, vector<vector<uint8_t>> &pixels) :
            m_width(width), m_height(height), m_pixels(pixels)
    {
        cout << "VideoFrame constructor called : " << this << endl;
    }
    /**
     * @brief video frame class destructor.
     */
    ~VideoFrame()
    {
        cout << "VideoFrame destructor called : " << this << endl;
    }
    uint8_t m_width; /**< width of frame . */
    uint8_t m_height; /**< Height of frame . */
    vector<vector<uint8_t>> m_pixels; /**< 2D vector of frame pixels. */
};

/**
 * @brief virtual class to define all common methods (pure virtual) to be able to create pipeline between elements.
 */
class Element
{
public:
    /**
     * @brief pure virtual method to link element.
     * @param element(in): output element to link.
     * @return element(out): pointer to linked element.
     */
    virtual Element* link(Element *element) = 0;
    /**
     * @brief pure virtual method to process frame.
     * @param videoFrame(in): shared pointer of video frame.
     * @return void.
     */
    virtual void process(shared_ptr<VideoFrame> videoFrame) = 0;
    /**
     * @brief pure virtual method to process frame and forward video frame handling to next element.
     * @param videoFrame(in): shared pointer of video frame.
     * @return void.
     */
    virtual void processAndPushDownstream(shared_ptr<VideoFrame> videoFrame) = 0;
};

/**
 * @brief base class for all elements of pipeline with implementation of generic methods (link, processAndPushDownstream).
 */
class BaseElement: public Element
{
private:
    vector<Element*> m_nextElements; /**< vector to store all next elements in pipeline. */

public:
    /**
     * @brief BaseElement constructor.
     */
    BaseElement() : m_nextElements{}
    {
    }
    /**
     * @brief BaseElement destructor.
     */
    virtual ~BaseElement() = default;

    /**
     * @brief implementation of link method. any new element to linked is pushed in internal vector.
     * @param element(in): output element to link.
     * @return element(out): pointer to linked element.
     */
    Element* link(Element *element) override
    {
        m_nextElements.push_back(element);
        return element;
    }

    /**
     * @brief implementation of processAndPushDownstream method.It loops in vector of next element to call
     * process method of any next element and then call processAndPushDownstream method.
     * @param videoFrame(in): shared pointer of video frame.
     * @return void.
     */
    virtual void processAndPushDownstream(shared_ptr<VideoFrame> videoFrame) override
    {
        for_each(m_nextElements.cbegin(), m_nextElements.cend(), [&videoFrame](const auto &element) {
            element->process(videoFrame); element->processAndPushDownstream(videoFrame);
        });
    }
    friend class AsynchronousQueue; /**<  for AsynchronousQueue we have to reimplement some methods and access to private
                                          members of private BasicElement members class. */
};

/**
 * @brief video source element class to randomly generate frame video with a specified frame rate, width and height.
 */
class VideoSourceElement: public BaseElement
{
public:
    /**
     * @brief VideoSourceElement constructor.
     * @param width(in): width of frames to randomly generate.
     * @param height(in): height of frames to randomly generate.
     * @param frameRate(in): frequency of frame generation expressed in number of frames per second).
     */
    VideoSourceElement(uint8_t &width, uint8_t &height, uint8_t &frameRate) :
            m_width(width), m_height(height), m_frameRate(frameRate), m_internalThread{}, m_runningState(false)
    {
    }
    /**
     * @brief VideoSourceElement Destructor.
     */
    ~VideoSourceElement()
    {
        stop();
    }

    /**
     * @brief starts thread to generate frames and then we will be blocked until join of thread (stop of it).
     */
    void start()
    {
        if (!m_runningState)
        {
            m_runningState = true;
            m_internalThread = thread(&VideoSourceElement::RandomVideoFramesGenerator, this);
        }

        if (m_internalThread.joinable())
            m_internalThread.join();
    }

    /**
     * @brief stops thread to generate frames if already started.
     */
    void stop ()
    {
        if (m_runningState)
        {
            m_runningState = false;
            if (m_internalThread.joinable())
                m_internalThread.join();
        }
    }

    /**
     * @brief first node in pipeline. Video source generates random frame and forward it for processing
     * to next element.
     * @param videoFrame(in): shared pointer of video frame.
     * @return void.
     */
    void process(shared_ptr<VideoFrame> videoFrame) override
    {
        processAndPushDownstream(videoFrame);
    }
private:
    uint8_t m_width; /**< width of frame . */
    uint8_t m_height;/**< height of frame . */
    uint8_t m_frameRate; /**< frame rate : frequency of frame generation expressed in frame per second. */
    thread m_internalThread;/**< thread used to generate random frames. */
    bool m_runningState;/**< boolean to keep status of thread running true : run false : down. */
    shared_ptr<VideoFrame> m_videoFrame;/**< shared pointer of video frame. */

    /**
     * @brief method executes inside thread to generate random frames.
     * it generates frame push frame to next element and sleep time according
     * to frame rate specified by user for example for framerate =10 frame per seconds
     * it will sleep 100 ms between each frame generation.
     * it can be interrupted by setting m_runningState to false
     * @param void.
     * @return void.
     */
    void RandomVideoFramesGenerator(void)
    {
        while (m_runningState)
        {
            processAndPushDownstream(GenerateVideoFrame());
            this_thread::sleep_for(chrono::milliseconds(1000 / m_frameRate));
        }
    }

    /**
     * @brief method to generate randomly Video Frame.
     * uniform_int_distribution is used in order to generate random value
     * in set {0, 1}. pixels are two colors encoded.
     * it fill shared pointer with generated frame video and returns shared pointer.
     * @param void.
     * @return VideoFrame : shared pointer with generated video frame.
     */
    shared_ptr<VideoFrame> GenerateVideoFrame(void)
    {
        random_device rd;  //Will be used to obtain a seed for the random number engine
        mt19937 gen(rd()); //Standard mersenne_twister_engine seeded with rd()
        uniform_int_distribution<> distrib(0, 1);

        cout << "\n New Frame Generated Width : " << unsigned(m_width) << " Height : " << unsigned(m_height) << "\n";

        vector<vector<uint8_t>> m_pixels;
        for (int i = 0; i < m_height; i++)
        {
            vector<uint8_t> line;
            for (int j = 0; j < m_width; j++)
            {
                line.push_back(distrib(gen));
                cout << unsigned(line[j]) << " ";
            }
            cout << "\n";
            m_pixels.push_back(line);
        }

        cout << "\n";
        cout << "GenerateVideoFrame before make shared counter : " << m_videoFrame.use_count() << " pointer " << m_videoFrame.get() << "\n";
        m_videoFrame = make_shared<VideoFrame>(m_width, m_height, m_pixels);

        return m_videoFrame;
    }
};

/**
 * @brief display element to process frame rate and display it in stdout.
 */
class DisplayElement: public BaseElement
{
public:
    /**
     * @brief DisplayElement Constructor.
     */
    DisplayElement()
    {
    }
    /**
     * @brief DisplayElement Destructor.
     */
    ~DisplayElement()
    {
    }

    /**
     * @brief process frame to print it in stdout (pixel 1 -> '+' 0 -> '.').
     * @param videoFrame(in): shared pointer of video frame.
     * @return void.
     */
    void process(shared_ptr<VideoFrame> videoFrame) override
    {
        printVideoFrame(videoFrame);
    }

    /**
     * @brief process frame to print it in stdout (pixel 1 -> '+' 0 -> '.').
     * @param videoFrame(in): shared pointer of video frame.
     * @return void.
     */
    void printVideoFrame(shared_ptr<VideoFrame> videoFrame)
    {
        cout << "\n New Frame Width : " << unsigned(videoFrame->m_width) << " Height : " << unsigned(videoFrame->m_height) << "\n";
        for_each(videoFrame->m_pixels.cbegin(), videoFrame->m_pixels.cend(), [](const auto &raw)
        {
            for_each(raw.cbegin(), raw.cend(), [](const auto &pixel) {
                cout << ((pixel == 2) ? "$ " : ((pixel == 1) ? "+ " :". "));
            });
            cout << "\n";
        });
        cout << "\n";
    }
};

/**
 * @brief detector element to find specific patterns in frame rate and mark it
 * for print with different value.
 */
class DetectorElement: public BaseElement
{
public:
    /**
     * @brief DetectorElement constructor.
     * @param pattern(in): pattern to find inside/mark it inside video frame.
     */
    DetectorElement(vector<vector<uint8_t>> &pattern) :
            m_patternToDetect(pattern)
    {
    }

    /**
     * @brief DetectorElement Destructor.
     */
    ~DetectorElement()
    {
    }

    /**
     * @brief to process frame video in order to find and mark specified pattern.
     * @param videoFrame(in): shared pointer of video frame.
     * @return void.
     */
    void process(shared_ptr<VideoFrame> videoFrame) override
    {
        checkPatternAndMarkExistingPatterns(videoFrame);
    }

private:

    /**
     * @brief once pattern found this method will alter values of pattern to change them
     * from 1 to 2 (note that 0 pixels of found pattern remains unchanged).
     * @param videoFrame(in): shared pointer of video frame.
     * @param xPosition(in): x position of found pattern.
     * @param yPosition(in): y position of found pattern.
     * @return void.
     */
    void markPattern(shared_ptr<VideoFrame> videoFrame, uint8_t xPosition, uint8_t yPosition)
    {
        for (auto k = yPosition; k < (yPosition + m_patternToDetect.size()); k++)
        {
            for (auto h = xPosition; h < (xPosition + m_patternToDetect[0].size()); h++)
            {
                videoFrame->m_pixels[k][h] = (videoFrame->m_pixels[k][h] > 0 ? 2 : videoFrame->m_pixels[k][h]);
            }
        }
    }

    /**
     * @brief Checks all occurrences of pattern in a video frame.
     * all found patterns are marked by changed 1 value to 2 (see markPattern method.)
     * @param videoFrame(in): shared pointer of video frame.
     * @return void.
     */
    void checkPatternAndMarkExistingPatterns(shared_ptr<VideoFrame> videoFrame)
    {
        vector<vector<uint8_t>> pixels = videoFrame->m_pixels;
        bool found = false;

        // checks if width or height of pattern bigger then video frame which means pattern cannot be found
        if ((pixels.size() < m_patternToDetect.size()) || (pixels[0].size() < m_patternToDetect[0].size()))
        {
            return;
        }

        for (int j = 0; j < (pixels.size() - m_patternToDetect.size()); j++)
        {
            // fix raw j from input frame video
            vector<uint8_t> raw = pixels[j];
            // try to search first raw of pattern in each raw
            vector<uint8_t> raw_pattern = m_patternToDetect[0];
            // loop in pixels of frame video raw to find first raw from pattern
            for (auto i = 0; i < (raw.size() - raw_pattern.size()); i++)
            {
                found = equal(m_patternToDetect[0].begin(), m_patternToDetect[0].end(), raw.begin() + i);
                // first raw found look to other raws from pattern in next raws from current frame video raw
                // in same positions j for raw and i for pixel position
                if (found)
                {
                    // loop here for all next element of pattern to check if they exists in next raws from video frame
                    for (auto k = 1; k < m_patternToDetect.size(); k++)
                    {
                        found = equal(m_patternToDetect[k].begin(), m_patternToDetect[k].end(), pixels[j + k].begin() + i);
                        // no found then continue looping
                        if (!found)
                            break;
                    }

                    // all elements from pattern found in following position (j, i)
                    if (found)
                    {
                        cout << "***** PATTERN FOUND AT POSITION j : " << j << " i : " << i << " ******\n";
                        // mark pattern for display with '$'
                        markPattern(videoFrame, i, j);
                    }
                }
            }
        }
    }
    vector<vector<uint8_t>> m_patternToDetect;/**< pattern to detect. */
};

/**
 * @brief Asynchronous queue element it will be inserted in pipeline in order
 * to avoid blocking pipeline processing by any heavy processing element(detector for example).
 */
class AsynchronousQueue: public BaseElement
{
public:
    /**
     * @brief AsynchronousQueue constructor
     * @param queueMaxSize(in): authorized max queue size in terms
     * of number of frames.
     * @return void.
     */
    AsynchronousQueue(uint8_t queueMaxSize) :
            m_runningState(false),
            m_startedState(false),
            m_internalThread{},
            m_queueMaxSize(queueMaxSize)
    {
    }

    /**
     * @brief AsynchronousQueue destructor it will
     * interrupt thread by setting running state to false
     * unblock it if thread is sleeping waiting for a conditional
     * variable notification.Also waits to join thread before empty
     * free all queue content.
     */
    ~AsynchronousQueue()
    {
        m_runningState = false;
        m_queueCv.notify_all();
        if (m_internalThread.joinable())
            m_internalThread.join();
        checkQueueSizeConstraint(0);
    }

    /**
     * @brief process frame : it will start thread for queue management
     * if not started and then add new frame in queue.
     * @param videoFrame(in): shared pointer of video frame.
     * @return void.
     */
    void process(shared_ptr<VideoFrame> videoFrame) override
    {
        start();
        pushNewVideoFrame(videoFrame);
    }

    /**
     * @brief overloaded nothing to do as pushdown stream will be
     * managed asynchronously by reading from queue in separate
     * thread.
     * @param videoFrame(in): shared pointer of video frame.
     * @return void.
     */
    void processAndPushDownstream(shared_ptr<VideoFrame> videoFrame) override
    {
        (void)videoFrame;
        return;
    }
private:
    bool m_runningState;/**< running state of thread . */
    bool m_startedState;/**< start state of thread : it differs from running state as we can interrupt
    started thread for example : to avoid start interrupted thread. */
    thread m_internalThread;/**< pattern to detect. */
    queue<shared_ptr<VideoFrame>> m_videoFramesQueue;/**< queue of video frame shared pointers. */
    mutex m_queueLock;/**< mutex to protect wakeup condition. */
    condition_variable m_queueCv;/**< conditional variable to wakeup read from queue. */
    uint8_t m_queueMaxSize;/**< max queue size. */

    /**
     * @brief starts asynchronous queue thread if not started.
     * @param videoFrame(in): shared pointer of video frame.
     */
    void start()
    {
        if (!m_startedState)
        {
            m_startedState = true;
            m_runningState = true;
            m_internalThread = thread{&AsynchronousQueue::waitForNewVideoFrame, this };
        }
    }

    /**
     * @brief checks queue size and deletes oldest frames when
     * size of queue exceeds maxSize specified in input.
     * @param maxSize(in): max authorize size of async queue.
     */
    void checkQueueSizeConstraint(uint8_t maxSize)
    {
        while (m_videoFramesQueue.size() > maxSize)
        {
            auto newVideoFrame = m_videoFramesQueue.front();//get oldest elemet from queue
            m_videoFramesQueue.pop(); //remove element from queue
            newVideoFrame.reset(); //free memory hold by shared pointer
        }
    }

    /**
     * @brief adds new frame in queue (at the end of queue).
     * it starts by checking queue size then push new frame
     * and then wakeup queue read to forward new frame
     * for next elements to process.
     * @param newVideoFrame(in): shared pointer of video frame.
     */
    void pushNewVideoFrame(shared_ptr<VideoFrame> newVideoFrame)
    {
        if (m_queueMaxSize > 0)
        {
            unique_lock<mutex> lock(m_queueLock);
            checkQueueSizeConstraint(m_queueMaxSize);
            m_videoFramesQueue.push(newVideoFrame);
            lock.unlock();
            m_queueCv.notify_one();
        }
    }

    /**
     * @brief method executed in thread of asynchronous queue.
     * it reads from the head of queue and forward for processing
     * to next element and then sleep waiting for new frames.
     */
    void waitForNewVideoFrame(void)
    {
        unique_lock<mutex> lock(m_queueLock);

        do
        {
            //Wait until new frame video added notified.
            m_queueCv.wait(lock, [this]
            {
                return (m_videoFramesQueue.size());
            });

            //after wait, we own the lock
            while(m_videoFramesQueue.size())
            {
                auto newVideoFrame = m_videoFramesQueue.front(); //get oldest elemet from queue
                m_videoFramesQueue.pop();//remove element from queue

                //unlock now as we already got a video frame from queue to process
                lock.unlock();

                if (!m_runningState)
                    break;

                // to notify next elements
                for_each(m_nextElements.cbegin(), m_nextElements.cend(), [&newVideoFrame](const auto &element)
                {
                    element->process(newVideoFrame); element->processAndPushDownstream(newVideoFrame);
                });

                lock.lock();
            }
        } while (m_runningState);
    }
};

/**
 * main of process
 */
int main(int argc,char* argv[])
{
    uint8_t width = 20;
    uint8_t height = 25;
    uint8_t frameRate = 1;
    VideoSourceElement *videoSourceElement = nullptr;
    DisplayElement *displayElement = nullptr;
    DetectorElement *detectorElement = nullptr;
    AsynchronousQueue *asynchQueue = nullptr;
    (void)asynchQueue;
    (void)detectorElement;

    try
    {
        videoSourceElement = new VideoSourceElement(width, height, frameRate);
        displayElement = new DisplayElement();
#if defined(DISPLAY_ONLY)
        videoSourceElement->link(displayElement);
        //    ******************           *******************
        //    *                *           *                 *
        //    *  VIDEO SOURCE  ***********>*     DISPLAY     *
        //    *                *           *                 *
        //    ******************           *******************
#elif defined(DISPLAY_WITH_DETECTOR)
        DetectorElement *detectorElement = new DetectorElement(pattern);
        videoSourceElement->link(detectorElement)->link(displayElement);
        //    ******************           *******************           *******************
        //    *                *           *                 *           *                 *
        //    *  VIDEO SOURCE  ***********>*     DETECTOR    ***********>*     DISPLAY     *
        //    *                *           *                 *           *                 *
        //    ******************           *******************           *******************
#else //to avoid that detector bloque display we use asynchronous queue for dispatching samples
        vector<vector<uint8_t>> pattern
        {
            { 0, 1, 0 }, // { . , + , .}
            { 1, 1, 1 }, // { + , + , +}
            { 0, 1, 0 }, // { . , + , .}
            { 1, 0, 1 }  // { + , . , +}
        };
        uint8_t asyncQueueMaxSize = 1;
        DetectorElement *detectorElement = new DetectorElement(pattern);
        AsynchronousQueue *asynchQueue = new AsynchronousQueue(asyncQueueMaxSize);

        //    ******************           *******************           *******************
        //    *                *           *                 *           *                 *
        //    *  VIDEO SOURCE  ***********>*   AsyncQueue    ***********>*     DETECTOR    *
        //    *                *     *     *                 *           *                 *
        //    ******************     *     *******************           *******************
        //                           *
        //                           *     *******************
        //                           *     *                 *
        //                           *****>*   DISPLAY       *
        //                                 *                 *
        //                                 *******************

        videoSourceElement->link(asynchQueue)->link(detectorElement);
        videoSourceElement->link(displayElement);
#endif
        videoSourceElement->start();

    } catch (const exception &e)
    {
        cout << "Unexpected exception: %s" << e.what();
        return -1;
    } catch (...)
    {
        cout << "Unexpected error";
        return -1;
    }

    delete videoSourceElement;
    delete displayElement;
    delete asynchQueue;
    delete detectorElement;

    return 0;
}
