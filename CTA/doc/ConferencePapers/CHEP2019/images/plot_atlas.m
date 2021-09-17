clear
close all

atlas_struct = load("atlas.mat");
atlas = atlas_struct.atlas;

date = atlas(:,6) + atlas(:,5) * 60 + atlas(:,4) * 3600 + atlas(:,3) * 86400;
date = date / 86400;

tpsrv010 = atlas(:,7);
tpsrv011 = atlas(:,8);
tpsrv012 = atlas(:,9);
tpsrv013 = atlas(:,10);
tpsrv103 = atlas(:,11);
tpsrv104 = atlas(:,12);
tpsrv108 = atlas(:,13);
tpsrv109 = atlas(:,14);
tpsrv112 = atlas(:,15);
tpsrv113 = atlas(:,16);
tpsrv114 = atlas(:,17);
tpsrv115 = atlas(:,18);
tpsrv116 = atlas(:,19);
tpsrv117 = atlas(:,20);
tpsrv118 = atlas(:,21);
tpsrv119 = atlas(:,22);
tpsrv210 = atlas(:,23);
tpsrv211 = atlas(:,24);
tpsrv213 = atlas(:,25);
tpsrv221 = atlas(:,26);
tpsrv222 = atlas(:,27);
tpsrv223 = atlas(:,28);
tpsrv224 = atlas(:,29);
tpsrv410 = atlas(:,30);

%plot(date,tpsrv010,'g','LineWidth',2)
ar = area(date,tpsrv010);
set(ar, 'EdgeColor', 'red');
set(ar, 'FaceColor', 'yellow');
set(ar, 'LineWidth', 2);


title('Title of this plot')
xlabel('August 2019')
ylabel('Y axis label')

print -dsvg figure1.svg
