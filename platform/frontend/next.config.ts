import type { NextConfig } from "next";

const nextConfig: NextConfig = {
  // 将前端相对路径 /api/* 代理到本地后端，避免 CORS 问题
  async rewrites() {
    return [
      {
        source: "/api/:path*",
        destination: "http://localhost:5080/api/:path*",
      },
    ];
  },
};

export default nextConfig;
