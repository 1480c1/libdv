# From cdda2wav, by way of Giovanni Iachello <g.iachello@iol.it>
# and Arne Schirmacher <arne@schirmacher.de>.
# Frequency response of the deemphasis. The error is below +-0.1dB
# first define the ideal filter. We use the tenfold sampling frequency.
T=1./441000.
OmegaU=1./15E-6
OmegaL=15./50.*OmegaU
V0=OmegaL/OmegaU
H0=V0-1.
B=V0*tan(OmegaU*T/2.)
# the coefficients follow
a1=(B - 1.)/(B + 1.)
b0=(1.0 + (1.0 - a1) * H0/2.)
b1=(a1 + (a1 - 1.0) * H0/2.)
# helper variables
D=b1/b0
o=2*pi*T
H2(f)=b0*sqrt((1+2*cos(f*o)*D+D*D)/(1+2*cos(f*o)*a1+a1*a1))
# now approximate the ideal curve with a fitted one for sampling frequency
# of 44100 Hz.
T2=1./44100.
V02=0.3365
OmegaU2=1./19E-6
B2=V02*tan(OmegaU2*T2/2.)
# the coefficients follow
a12=(B2 - 1.)/(B2 + 1.)
b02=(1.0 + (1.0 - a12) * (V02-1.)/2.)
b12=(a12 + (a12 - 1.0) * (V02-1.)/2.)
# helper variables
D2=b12/b02
o2=2*pi*T2
H(f)=b02*sqrt((1+2*cos(f*o2)*D2+D2*D2)/(1+2*cos(f*o2)*a12+a12*a12))
# plot best, real, ideal, level with halved attenuation,
#      level at full attentuation, 10fold magnified error
set logscale x
set grid xtics ytics mxtics mytics
plot [f=1000:20000] [-12:2] 20*log10(H(f)),20*log10(H2(f)),  20*log10(OmegaL/(2*pi*f)), 0.5*20*log10(V0), 20*log10(V0), 200*log10(H(f)/H2(f))
pause -1 "Hit return to continue"
