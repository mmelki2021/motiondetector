#!/usr/bin/env python3.12

import threading
import queue
import random
import time

class VideoFrame:
    def __init__(self, width, height, pixels):
        self.width = width
        self.height = height
        self.pixels = pixels
        print(f"VideoFrame created: {self}")

    def __del__(self):
        print(f"VideoFrame destroyed: {self}")

class Element:
    def link(self, element):
        raise NotImplementedError

    def process(self, video_frame):
        raise NotImplementedError

    def process_and_push_downstream(self, video_frame):
        raise NotImplementedError

class BaseElement(Element):
    def __init__(self):
        self.next_elements = []

    def link(self, element):
        self.next_elements.append(element)
        return element

    def process_and_push_downstream(self, video_frame):
        for element in self.next_elements:
            element.process(video_frame)
            element.process_and_push_downstream(video_frame)

class VideoSourceElement(BaseElement):
    def __init__(self, width, height, frame_rate):
        super().__init__()
        self.width = width
        self.height = height
        self.frame_rate = frame_rate
        self.running_state = False
        self.internal_thread = None

    def start(self):
        if not self.running_state:
            self.running_state = True
            self.internal_thread = threading.Thread(target=self.random_video_frames_generator)
            self.internal_thread.start()

    def stop(self):
        self.running_state = False
        if self.internal_thread:
            self.internal_thread.join()

    def process(self, video_frame):
        self.process_and_push_downstream(video_frame)

    def random_video_frames_generator(self):
        while self.running_state:
            frame = self.generate_video_frame()
            self.process_and_push_downstream(frame)
            time.sleep(1 / self.frame_rate)

    def generate_video_frame(self):
        pixels = [[random.randint(0, 1) for _ in range(self.width)] for _ in range(self.height)]
        print(f"\nNew Frame Generated Width: {self.width} Height: {self.height}")
        for row in pixels:
            print(" ".join(str(p) for p in row))
        return VideoFrame(self.width, self.height, pixels)

class DisplayElement(BaseElement):
    def process(self, video_frame):
        self.print_video_frame(video_frame)

    def print_video_frame(self, video_frame):
        print(f"\nNew Frame Width: {video_frame.width} Height: {video_frame.height}")
        for row in video_frame.pixels:
            print(" ".join("+" if p == 1 else "|" if p == 2 else "." for p in row))
        print("\n")

class DetectorElement(BaseElement):
    def __init__(self, pattern):
        super().__init__()
        self.pattern_to_detect = pattern

    def process(self, video_frame):
        self.check_pattern_and_mark_existing_patterns(video_frame)

    def mark_pattern(self, video_frame, x, y):
        for k in range(len(self.pattern_to_detect)):
            for h in range(len(self.pattern_to_detect[0])):
                if video_frame.pixels[y + k][x + h] > 0:
                    video_frame.pixels[y + k][x + h] = 2

    def check_pattern_and_mark_existing_patterns(self, video_frame):
        pixels = video_frame.pixels
        height, width = len(pixels), len(pixels[0])
        p_height, p_width = len(self.pattern_to_detect), len(self.pattern_to_detect[0])

        if height < p_height or width < p_width:
            return

        for j in range(height - p_height + 1):
            for i in range(width - p_width + 1):
                found = all(
                    pixels[j + k][i:i + p_width] == self.pattern_to_detect[k]
                    for k in range(p_height)
                )
                if found:
                    print(f"***** PATTERN FOUND AT POSITION ({j}, {i}) ******")
                    self.mark_pattern(video_frame, i, j)

class AsynchronousQueue(BaseElement):
    def __init__(self, queue_max_size):
        super().__init__()
        self.queue = queue.Queue(maxsize=queue_max_size)
        self.running_state = False
        self.thread = None

    def process(self, video_frame):
        if not self.running_state:
            self.running_state = True
            self.thread = threading.Thread(target=self.wait_for_new_video_frame)
            self.thread.start()
        self.queue.put(video_frame)

    def process_and_push_downstream(self, video_frame):
        pass  # La gestion est faite via le thread

    def wait_for_new_video_frame(self):
        while self.running_state:
            video_frame = self.queue.get()
            for element in self.next_elements:
                element.process(video_frame)
                element.process_and_push_downstream(video_frame)

# Exemple d'utilisation
if __name__ == "__main__":
    width, height, frame_rate = 20, 20, 2
    pattern =  [[ 0, 1, 0 ],  # { . , + , .}
                [ 1, 1, 1 ], # { + , + , +}
                [ 0, 1, 0 ], # { . , + , .}
                [ 1, 0, 1 ]]  #{{ + , . , +}

    source = VideoSourceElement(width, height, frame_rate)
    queue_element = AsynchronousQueue(10)
    detector = DetectorElement(pattern)
    display = DisplayElement()

    source.link(queue_element).link(detector).link(display)
    source.start()
    time.sleep(50)  # Exécuter pendant quelques secondes
    source.stop()