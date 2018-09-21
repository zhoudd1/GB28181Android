package com.autulin.gb28181library;

public class MediaOutput {
    private String ip;
    private int port;
    private String outputDir;
    private String outputName;
    private int outputType;
    private int ssrc;

    public MediaOutput(String ip, int port, String outputDir, String outputName, int outputType, int ssrc) {
        this.ip = ip;
        this.port = port;
        this.outputDir = outputDir;
        this.outputName = outputName;
        this.outputType = outputType;
        this.ssrc = ssrc;
    }

    public String getIp() {
        return ip;
    }

    public void setIp(String ip) {
        this.ip = ip;
    }

    public int getPort() {
        return port;
    }

    public void setPort(int port) {
        this.port = port;
    }

    public String getOutputDir() {
        return outputDir;
    }

    public void setOutputDir(String outputDir) {
        this.outputDir = outputDir;
    }

    public String getOutputName() {
        return outputName;
    }

    public void setOutputName(String outputName) {
        this.outputName = outputName;
    }

    public int getOutputType() {
        return outputType;
    }

    public void setOutputType(int outputType) {
        this.outputType = outputType;
    }

    public int getSsrc() {
        return ssrc;
    }

    public void setSsrc(int ssrc) {
        this.ssrc = ssrc;
    }
}
