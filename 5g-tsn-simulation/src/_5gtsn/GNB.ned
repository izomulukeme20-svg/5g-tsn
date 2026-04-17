package src._5gtsn;

simple GNB
{
    parameters:
        int gnbId = default(0);
        int numUes = default(2);
        double frequency @unit(Hz) = default(5.9GHz);
        double bandwidth @unit(Hz) = default(100MHz);
        double txPower @unit(dBm) = default(46dBm);
        double antennaGain @unit(dB) = default(10dB);
        double noiseFigure @unit(dB) = default(5dB);
        double jitterStd @unit(s) = default(0.0005s);
        double ueVelocity @unit(mps) = default(3mps);
        @display("i=device/antennatower;is=l");
    gates:
        input ueIn[] @labels(radio);
        output ueOut[] @labels(radio);
        input coreIn @labels(EtherFrame-conn);
        output coreOut @labels(EtherFrame-conn);
}
