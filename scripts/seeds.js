#!/usr/bin/env node

const program = require('commander')
const compile = require('./compile')
const deploy = require('./deploy.command')

program
  .command('compile <contract>')
  .description('Compile custom contract')
  .action(async function (contract) {
    await compile({
      contract: contract,
      source: `./src/seeds.${contract}.cpp`
    })
    
    console.log(`${contract} compiled`)
  })

program
  .command('deploy <contract>')
  .description('Deploy custom contract')
  .action(async function (contract) {
    await deploy(contract)
    console.log(`${contract} deployed`)
  })
  
program.parse(process.argv)