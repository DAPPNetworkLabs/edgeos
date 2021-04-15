const path = require('path');
const webpack = require('webpack');
const HtmlWebpackPlugin = require('html-webpack-plugin');
// const ExtractTextPlugin = require('extract-text-webpack-plugin');

module.exports = {
    mode: 'development',
    entry: {
        bundle:'./src/index.ts',
        EdgeOSWASMWorker:'./src/EdgeOSWASMWorker.js',
    },
    resolve: {
        extensions: ['.ts', '.js']
    },
    output: {
        filename: '[name].js',
        path: path.resolve(__dirname, 'dist')
    },
    module: {
        rules: [
            {
                test: /\.abi$/,
                loader: 'json-loader'
              },
            // {
            //     enforce: 'pre',
            //     test: /\.js$/,
            //     loader: "source-map-loader"
            // },
            {
                test: /\.ts$/,
                loader: 'ts-loader',
                exclude: /node_modules/
            },
            {
                
                test: /bundle\.js$/,
                loader: 'string-replace-loader',
                options: {
                    search: /(.)\.fakePath/g,
                    replace: (match, p1, offset, string) => `${p1}.fakePath || ${p1}.path`,
                }
                
            }
        ]
    },
    plugins: [
        new HtmlWebpackPlugin({
            template: 'src/index.html',
            chunks:['bundle']
        })
    ],
    // devtool: "source-map"
};
