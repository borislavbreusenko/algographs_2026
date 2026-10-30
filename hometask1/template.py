import signal
import sys

stop = False


def on_sigterm(signum, frame):
    global stop
    stop = True


def read_graph():
    lines = sys.stdin.buffer.read().split(b'\n')
    n = int(lines[0].split()[0])
    adj = [list(map(int, line.split())) for line in lines[1:n + 1]]
    return n, adj


def main():
    signal.signal(signal.SIGTERM, on_sigterm)
    n, adj = read_graph()

    best = [1]
    while not stop:
        pass

    sys.stdout.write(''.join(f'{v}\n' for v in best))
    sys.stdout.flush()


if __name__ == '__main__':
    main()
