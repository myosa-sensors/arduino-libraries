"""
Synopsis: Build the compressed ESP32 Wi-Fi dashboard.
Run with Python 3 after editing dashboard.html; no third-party packages are needed.

All information, including URL references, is subject to change without prior notice.
Please always use the latest versions of software-release for best performance.
Unless required by applicable law or agreed to in writing, this software is distributed on an
"AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.

Modifications
20 October, 2026 by Pegasus Automation
(as a part of MYOSA Initiative)
Contact Team MYOSA
Email: myosa.event@gmail.com
"""

from pathlib import Path
import gzip


def main():
    folder = Path(__file__).resolve().parent
    source = (folder / 'dashboard.html').read_text(encoding='utf-8')
    wifi = source.encode('utf-8')
    # Fix the gzip timestamp and header to keep identical input reproducible.
    compressed = bytearray(gzip.compress(wifi, compresslevel=9, mtime=0))
    compressed[9] = 255
    notice = source.split('<!--', 1)[1].split('-->', 1)[0]
    header = '/*' + notice + '*/\n\n#ifndef MYOSA_DASHBOARD_H\n#define MYOSA_DASHBOARD_H\n\n'
    header += '/* Generated from extras/web/dashboard.html; run build_dashboard.py to refresh. */\n'
    header += 'static const uint8_t MYOSA_DASHBOARD[] PROGMEM = {\n'
    for offset in range(0, len(compressed), 16):
        header += '  ' + ', '.join('0x%02X' % value for value in compressed[offset:offset + 16]) + ',\n'
    header += '};\n\n#endif\n'
    (folder.parent.parent / 'src' / 'myosaDashboard.h').write_text(header, encoding='utf-8', newline='\n')
    print('Wi-Fi page: %d bytes; compressed: %d bytes.' % (len(wifi), len(compressed)))


if __name__ == '__main__':
    main()
