clear
close all

atlas = dlmread("Atlas_2018_reprocessing_retrieve-3.1-comma.csv", ",");

date = atlas(:,6) + atlas(:,5) * 60 + atlas(:,4) * 3600 + atlas(:,3) * 86400;
date = date / 86400;

tpsrvs = atlas(:,7:28);

figure(1, 'position', [300, 300, 700, 400]);
ar = area(date,tpsrvs/1e9);
axis([13, 16]);
grid on;

pf=500;
off=(pf-256)/pf;

palette = [ [ 3, 22, 38 ]/pf+off
[ 4, 32, 57 ]/pf+off
[ 5, 43, 76 ]/pf+off
[ 7, 54, 95 ]/pf+off
[ 8, 65, 114 ]/pf+off
[ 9, 76, 134 ]/pf+off
[ 11, 86, 153 ]/pf+off
[ 12, 97, 172 ]/pf+off
[ 13, 108, 191 ]/pf+off
[ 15, 119, 210 ]/pf+off
[ 16, 129, 229 ]/pf+off
[ 26, 140, 239 ]/pf+off
[ 40, 146, 240 ]/pf+off
[ 64, 159, 242 ]/pf+off
[ 83, 169, 243 ]/pf+off
[ 102, 178, 244 ]/pf+off
[ 121, 188, 246 ]/pf+off
[ 141, 197, 247 ]/pf+off
[ 160, 207, 248 ]/pf+off
[ 179, 217, 250 ]/pf+off
[ 198, 226, 251 ]/pf+off
[ 217, 236, 252 ]/pf+off
[ 236, 245, 254 ]/pf+off
[ 255, 255, 255 ]/pf+off ];

for i = 1:22
  set(ar(i), 'FaceColor', palette(i,:))
endfor
colormap hsv;
title('Tape servers bandwidth (30 min avg, stacked)')
xlabel('Days of August 2019')
ylabel('Bandwidth (GB/s)')

% -S in points
print -dpdf -tight "-S600,320" Atlas_2018_reprocessing_tape_bandwidth.pdf
