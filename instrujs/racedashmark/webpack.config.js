//Webpack requires this to work with directories
const path =  require('path')
const webpack = require('webpack')
const Extract = require('mini-css-extract-plugin')
const Compresseur = require('terser-webpack-plugin')
const HtmlInstaller = require('html-webpack-plugin')

// This is main configuration object that tells Webpackw what to do.
module.exports = {
    //path to entry paint
    entry: {
        main: path.resolve(__dirname, './src/index.ts')
    },
    //path and filename of the final output
    output: {
        path: path.resolve(__dirname, '../../data/instrujs/www/racedashmark'),
        filename: 'bundle.js'
    },
    module: {
        rules: [
            {
                test: /\.ts?$/,
                loader: ['awesome-typescript-loader'],
                include: [path.resolve(__dirname, './src'),
                          path.resolve(__dirname, '../src')
                         ],
                exclude: [/(node_modules)/]
            },
            {
                test: /\.js$/,
                loader: 'babel-loader',
                include: [path.resolve(__dirname, './src'),
                          path.resolve(__dirname, '../src'),
                          path.resolve(__dirname, './node_modules/bootstrap-sass/assets/javascripts/bootstrap')
                         ],
                exclude: [/(node_modules)/],
                options: {
                    "presets": [
                      ["@babel/preset-env", {
                          "useBuiltIns": "entry",
                          "corejs": {"version": 3, "proposals": true},
                          "debug": true,
                          "targets": {
                              "ie": "11",
                              "safari": "6"
                          }
                      }]
                    ],
                    plugins: ['@babel/plugin-transform-runtime'],
                    cacheDirectory: true
                }
            },
            {
                test:/\.(sa|sc|c)ss$/,
                exclude: /(node_modules)/,
                include: [path.resolve(__dirname, './sass'),
                          path.resolve(__dirname, '../sass')],
                use: [
                    {
                        loader: Extract.loader
                    },
                    {
                        loader: 'css-loader'
                    },
                    {
                        loader: 'sass-loader',
                        options: {
                            implementation: require('sass')
                        }
                    }
                ]
            },
            {
                test: /\.(png|jpe?g|gif|svg)$/,
                include: [path.resolve(__dirname, '../image')],
                exclude: /(node_modules)/,
                use: [
                    {
                        loader: 'file-loader',
                        options: {
                            outputPath: 'images'
                        }
                    }
                ]
            },
            {
                test: /iface\.js$/,
                include: [path.resolve(__dirname, '../src')],
                exclude: /(node_modules)/,
                use: [
                    {
                        loader: 'exports-loader'
                    },
                ]
            },
        ],
    },
    resolve: {
      extensions: ['.js', '.ts', '.tsx']
    },
    optimization: {
        minimizer: [
            new Compresseur({
                cache: true,
                parallel: true,
                sourceMap: true
            }),
        ],
    },
    devtool: 'source-map',
    stats: {
        children: false,
        maxModules: 0
    },
    performance: {
        hints: false
    },
    plugins: [
        new Extract({
            filename: 'bundle.css'
        }),
        new HtmlInstaller({
            template: './html/index.html',
            filename: 'index.html'
        }),
        new webpack.ProvidePlugin({
            $: "jquery",
            jQuery: "jquery"
        }),
        new webpack.ProvidePlugin({
            $: "bootstrap-sass",
            Bootstrap: "bootstrap-sass"
        })
    ],
    //default mode is production
    mode: 'development'
}
